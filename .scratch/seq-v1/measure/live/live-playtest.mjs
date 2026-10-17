// Plays the set for a few seconds and samples one track's output meter, then stops and puts the song position back.
//   node live-playtest.mjs <track index> <seconds>
import { Bridge, DEFAULT_LIVE_PORT } from "file:///C:/dev/tuple/tools/live-mcp/bridge.mjs";

const [trackIndex, seconds] = process.argv.slice(2).map(Number);
const live = new Bridge({ name: "Live", port: DEFAULT_LIVE_PORT });
const get = (path, prop) => live.request("get", { path, prop });
const call = (path, method, args = []) => live.request("call", { path, method, args });
const set = (path, prop, value) => live.request("set", { path, prop, value });
const sleep = ms => new Promise(r => setTimeout(r, ms));

const track = `live_set tracks ${trackIndex}`;
let start = null;
try
{
	start = await get("live_set", "current_song_time");
	console.log("start position", start, "tempo", await get("live_set", "tempo"));
	await call("live_set", "start_playing");
	const t0 = Date.now();
	const samples = [];
	while(Date.now() - t0 < seconds * 1000)
	{
		const level = await get(track, "output_meter_level");
		samples.push([Date.now() - t0, level]);
		await sleep(30);
	}
	console.log("playing at end", await get("live_set", "is_playing"), "position", await get("live_set", "current_song_time"));
	const loud = samples.filter(([, l]) => l > 0.001);
	console.log("samples", samples.length, "above 0.001:", loud.length, "max", Math.max(...samples.map(([, l]) => l)).toFixed(3));
	console.log(samples.map(([t, l]) => t + ":" + l.toFixed(2)).join(" "));
}
catch(error)
{
	console.error(String(error?.message || error));
	process.exitCode = 1;
}
finally
{
	try
	{
		await call("live_set", "stop_playing");
		if(start !== null)
			await set("live_set", "current_song_time", start);
		console.log("stopped, position back to", await get("live_set", "current_song_time"));
	}
	catch(error)
	{
		console.error("cleanup:", String(error?.message || error));
	}
	live.close();
}
