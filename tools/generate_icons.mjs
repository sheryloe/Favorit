import fs from "node:fs/promises";
import https from "node:https";
import path from "node:path";
import sharp from "sharp";
import { fileURLToPath } from "node:url";

const __dirname = path.dirname(fileURLToPath(import.meta.url));
const rootDir = path.resolve(__dirname, "..");
const iconDir = path.join(rootDir, "assets", "icons");
const licenseDir = path.join(rootDir, "assets", "licenses");

const icons = {
  star: "star",
  settings: "settings-2",
  search: "search",
  save: "save",
  reset: "rotate-ccw",
  delete: "trash-2",
  app: "monitor",
  document: "file-text",
  link: "globe",
  external: "external-link",
};

function fetchText(url) {
  return new Promise((resolve, reject) => {
    https
      .get(url, (response) => {
        if (response.statusCode !== 200) {
          reject(new Error(`HTTP ${response.statusCode} for ${url}`));
          response.resume();
          return;
        }

        const chunks = [];
        response.on("data", (chunk) => chunks.push(chunk));
        response.on("end", () => resolve(Buffer.concat(chunks).toString("utf8")));
      })
      .on("error", reject);
  });
}

await fs.mkdir(iconDir, { recursive: true });
await fs.mkdir(licenseDir, { recursive: true });

for (const [outputName, iconName] of Object.entries(icons)) {
  const url = `https://raw.githubusercontent.com/lucide-icons/lucide/main/icons/${iconName}.svg`;
  const svg = (await fetchText(url)).replaceAll("currentColor", "white");
  const outputPath = path.join(iconDir, `${outputName}.png`);

  await sharp(Buffer.from(svg), { density: 360 })
    .resize(128, 128, { fit: "contain" })
    .png()
    .toFile(outputPath);
}

const license = await fetchText("https://raw.githubusercontent.com/lucide-icons/lucide/main/LICENSE");
await fs.writeFile(path.join(licenseDir, "lucide-icons.txt"), license, "utf8");
