// A track's mixer volume, read or set (0-1, Live's own scale; 0.85 is 0 dB): to keep a recording off the speakers,
// since live-record-track.mjs takes the track Post FX, before its fader.
//   node live-track-volume.mjs <track name> [<value>]
import { Bridge, DEFAULT_LIVE_PORT } from "file:///C:/dev/tuple/tools/live-mcp/bridge.mjs";

const [name, value] = process.argv.slice(2);
const live = new Bridge({ name: "Live", port: DEFAULT_LIVE_PORT });
const get = (path, prop) => live.request("get", { path, prop });
const set = (path, prop, v) => live.request("set", { path, prop, value: v });

try
{
	const tracks = await get("live_set", "tracks");
	const index = tracks.findIndex(t => t.name === name);
	if(index < 0)
		throw new Error("no track " + name);
	const volume = `live_set tracks ${index} mixer_device volume`;
	console.log("volume was", await get(volume, "value"));
	if(value !== undefined)
	{
		await set(volume, "value", Number(value));
		console.log("volume now", await get(volume, "value"));
	}
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
