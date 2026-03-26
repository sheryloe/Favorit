#!/usr/bin/env node

import { createServer } from "node:http";
import { createReadStream } from "node:fs";
import { mkdir, stat, writeFile } from "node:fs/promises";
import path from "node:path";
import { fileURLToPath } from "node:url";

import { chromium, devices } from "playwright";

const __dirname = path.dirname(fileURLToPath(import.meta.url));
const repoRoot = path.resolve(__dirname, "..", "..");
const docsRoot = path.join(repoRoot, "docs");
const outputRoot = path.join(repoRoot, "build", "playwright");
const port = Number(process.env.PLAYWRIGHT_AUDIT_PORT ?? 4280);
const origin = `http://127.0.0.1:${port}`;
const pageUrl = `${origin}/index.html`;

const mimeTypes = {
    ".css": "text/css; charset=utf-8",
    ".exe": "application/vnd.microsoft.portable-executable",
    ".html": "text/html; charset=utf-8",
    ".js": "text/javascript; charset=utf-8",
    ".json": "application/json; charset=utf-8",
    ".png": "image/png",
    ".svg": "image/svg+xml",
    ".txt": "text/plain; charset=utf-8",
    ".xml": "application/xml; charset=utf-8"
};

function contentTypeFor(filePath) {
    return mimeTypes[path.extname(filePath).toLowerCase()] ?? "application/octet-stream";
}

async function resolvePath(requestPath) {
    const cleanPath = requestPath === "/" ? "/index.html" : requestPath;
    const absolutePath = path.resolve(docsRoot, `.${cleanPath}`);
    if (!absolutePath.startsWith(docsRoot)) {
        return null;
    }

    let filePath = absolutePath;
    const fileStat = await stat(filePath).catch(() => null);
    if (fileStat?.isDirectory()) {
        filePath = path.join(filePath, "index.html");
    }

    const finalStat = await stat(filePath).catch(() => null);
    if (!finalStat?.isFile()) {
        return null;
    }

    return filePath;
}

async function startStaticServer() {
    const server = createServer(async (request, response) => {
        try {
            const requestUrl = new URL(request.url ?? "/", origin);
            const filePath = await resolvePath(decodeURIComponent(requestUrl.pathname));
            if (filePath === null) {
                response.writeHead(404, { "Content-Type": "text/plain; charset=utf-8" });
                response.end("not found");
                return;
            }

            response.writeHead(200, { "Content-Type": contentTypeFor(filePath) });
            createReadStream(filePath).pipe(response);
        } catch (error) {
            response.writeHead(500, { "Content-Type": "text/plain; charset=utf-8" });
            response.end(String(error));
        }
    });

    await new Promise((resolve, reject) => {
        server.once("error", reject);
        server.listen(port, "127.0.0.1", resolve);
    });

    return server;
}

async function fetchStatus(url) {
    const response = await fetch(url);
    return response.status;
}

function pushIssue(target, message, condition) {
    if (!condition) {
        target.push(message);
    }
}

async function auditView(browser, view) {
    const context = await browser.newContext(view.context);
    const page = await context.newPage();
    const consoleErrors = [];
    const pageErrors = [];

    page.on("console", (message) => {
        if (message.type() === "error") {
            consoleErrors.push(message.text());
        }
    });

    page.on("pageerror", (error) => {
        pageErrors.push(error.message);
    });

    await page.goto(pageUrl, { waitUntil: "networkidle" });

    const assetStatuses = {
        main: await fetchStatus(`${origin}/assets/screenshots/favorite-widget-main.png`),
        settings: await fetchStatus(`${origin}/assets/screenshots/favorite-widget-settings.png`)
    };

    const layout = await page.evaluate(() => {
        const rectFor = (selector) => {
            const element = document.querySelector(selector);
            if (element === null) {
                return null;
            }

            const rect = element.getBoundingClientRect();
            return {
                left: rect.left,
                top: rect.top,
                right: rect.right,
                bottom: rect.bottom,
                width: rect.width,
                height: rect.height
            };
        };

        const imageFor = (suffix) => {
            const element = document.querySelector(`img[src$="${suffix}"]`);
            if (element === null) {
                return null;
            }

            return {
                complete: element.complete,
                naturalWidth: element.naturalWidth,
                naturalHeight: element.naturalHeight
            };
        };

        return {
            hero: rectFor(".hero-visual"),
            mainCard: rectFor(".visual-main"),
            settingsCard: rectFor(".visual-settings"),
            mainImage: imageFor("favorite-widget-main.png"),
            settingsImage: imageFor("favorite-widget-settings.png")
        };
    });

    await page.screenshot({
        fullPage: true,
        path: path.join(outputRoot, `${view.name}.png`),
        scale: "css",
        type: "png"
    });

    await context.close();

    const issues = [];
    const hero = layout.hero;
    const mainCard = layout.mainCard;
    const settingsCard = layout.settingsCard;
    const tolerance = 1;

    pushIssue(issues, `[${view.name}] main screenshot status is ${assetStatuses.main}`, assetStatuses.main === 200);
    pushIssue(issues, `[${view.name}] settings screenshot status is ${assetStatuses.settings}`, assetStatuses.settings === 200);
    pushIssue(issues, `[${view.name}] hero visual block is missing`, hero !== null);
    pushIssue(issues, `[${view.name}] main hero card is missing`, mainCard !== null);
    pushIssue(issues, `[${view.name}] settings hero card is missing`, settingsCard !== null);
    pushIssue(issues, `[${view.name}] main image failed to load`, layout.mainImage?.complete === true && layout.mainImage?.naturalWidth > 0);
    pushIssue(issues, `[${view.name}] settings image failed to load`, layout.settingsImage?.complete === true && layout.settingsImage?.naturalWidth > 0);

    if (hero !== null && mainCard !== null) {
        pushIssue(issues, `[${view.name}] main hero card overflows the hero container`, mainCard.left >= hero.left - tolerance && mainCard.top >= hero.top - tolerance && mainCard.right <= hero.right + tolerance && mainCard.bottom <= hero.bottom + tolerance);
        pushIssue(issues, `[${view.name}] main hero card is too wide for the intended framing`, mainCard.width <= hero.width * view.maxMainWidthRatio);
    }

    if (hero !== null && settingsCard !== null) {
        pushIssue(issues, `[${view.name}] settings hero card overflows the hero container`, settingsCard.left >= hero.left - tolerance && settingsCard.top >= hero.top - tolerance && settingsCard.right <= hero.right + tolerance && settingsCard.bottom <= hero.bottom + tolerance);
        pushIssue(issues, `[${view.name}] settings hero card is too wide for the intended framing`, settingsCard.width <= hero.width * view.maxSettingsWidthRatio);
    }

    for (const error of consoleErrors) {
        issues.push(`[${view.name}] console error: ${error}`);
    }

    for (const error of pageErrors) {
        issues.push(`[${view.name}] page error: ${error}`);
    }

    return {
        name: view.name,
        assetStatuses,
        consoleErrors,
        pageErrors,
        layout,
        issues
    };
}

async function main() {
    await mkdir(outputRoot, { recursive: true });

    const server = await startStaticServer();
    const browser = await chromium.launch({
        args: ["--no-sandbox"],
        headless: true
    });

    const views = [
        {
            name: "desktop-home",
            context: {
                viewport: { width: 1440, height: 1200 },
                deviceScaleFactor: 1
            },
            maxMainWidthRatio: 0.86,
            maxSettingsWidthRatio: 0.76
        },
        {
            name: "mobile-home",
            context: {
                ...devices["iPhone 13"],
                viewport: { width: 390, height: 844 }
            },
            maxMainWidthRatio: 1.02,
            maxSettingsWidthRatio: 0.96
        }
    ];

    try {
        const results = [];
        for (const view of views) {
            results.push(await auditView(browser, view));
        }

        const reportPath = path.join(outputRoot, "report.json");
        await writeFile(reportPath, `${JSON.stringify({ origin, pageUrl, results }, null, 2)}\n`, "utf8");

        const issues = results.flatMap((result) => result.issues);
        for (const result of results) {
            console.log(`${result.name}: main=${result.assetStatuses.main}, settings=${result.assetStatuses.settings}, consoleErrors=${result.consoleErrors.length}, pageErrors=${result.pageErrors.length}`);
        }

        if (issues.length > 0) {
            for (const issue of issues) {
                console.error(issue);
            }
            process.exitCode = 1;
            return;
        }

        console.log(`report:${reportPath}`);
    } finally {
        await browser.close();
        await new Promise((resolve, reject) => {
            server.close((error) => {
                if (error) {
                    reject(error);
                    return;
                }
                resolve();
            });
        });
    }
}

main().catch((error) => {
    console.error(error);
    process.exitCode = 1;
});
