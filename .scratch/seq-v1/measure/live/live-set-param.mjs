// Sets a device parameter by its name, on the first device of a track whose name checks.
//   node live-set-param.mjs <track name> <device name> <parameter name> <value>
import { Bridge, DEFAULT_LIVE_PORT } from "file:///C:/dev/tuple/tools/live-mcp/bridge.mjs";

const [trackName, deviceName, paramName, value] = process.argv.slice(2);
const live = new Bridge({ name: "Live", port: DEFAULT_LIVE_PORT });
const get = (path, prop) => live.request("get", { path, prop });
const set = (path, prop, v) => live.request("set", { path, prop, value: v });

try
{
	const tracks = await get("live_set", "tracks");
	const t = tracks.findIndex(x => x.name === trackName);
	if(t < 0)
		throw new Error("no track " + trackName);
	const devices = await get(`live_set tracks ${t}`, "devices");
	const d = devices.findIndex(x => x.name === deviceName);
	if(d < 0)
		throw new Error("no device " + deviceName + " on " + trackName);
	const params = await get(`live_set tracks ${t} devices ${d}`, "parameters");
	const p = params.findIndex(x => x.name === paramName);
	if(p < 0)
		throw new Error("no parameter " + paramName + ": " + params.map(x => x.name).join(", "));
	const path = `live_set tracks ${t} devices ${d} parameters ${p}`;
	await set(path, "value", Number(value));
	console.log(paramName, "now", await get(path, "value"));
}
catch(error)
{
	console.error(String(error?.message || error));
	process.exitCode = 1;
}
finally
{
	live.close();
}
