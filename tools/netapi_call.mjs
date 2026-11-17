// Appel brut NET API Workbench. usage: node netapi_call.mjs <APIFunc> [jsonParams] [port=5775]
import { encodeRequest, decodeResponse } from "file:///C:/Users/benbo/enfusion-mcp-local/enfusion-mcp-BK-src/dist/workbench/protocol.js";
import { Socket } from "node:net";
const [fn, p = "{}", port = "5775"] = process.argv.slice(2);
const s = new Socket(); const chunks = [];
s.setTimeout(8000, () => { console.log("TIMEOUT"); s.destroy(); });
s.on("error", e => console.log("ERR", e.code || e.message));
s.on("data", c => { chunks.push(c); const b = Buffer.concat(chunks); try { console.log(JSON.stringify(decodeResponse(b))); s.destroy(); } catch (e) {} });
s.on("end", () => { const b = Buffer.concat(chunks); try { console.log(JSON.stringify(decodeResponse(b))); } catch (e) { console.log("RAW", b.toString("utf8").slice(0, 500)); } });
s.connect(+port, "127.0.0.1", () => s.write(encodeRequest("ZimniTest", fn, JSON.parse(p))));
