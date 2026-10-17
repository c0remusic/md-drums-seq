// Issue 24's scenario: two development devices on one track in front of MD Drums, each on the dense Pattern
// (%TEMP%\mdseq-demo.txt holding "dense"), the transport started. Run only on a test set the user accepts to see crash.
//   node live-crash-test.mjs setup <MD Drums path> <device path>   builds "MDSEQ CRASH", its fader at 0
//   node live-crash-test.mjs run <seconds>                          plays from the start, stops, puts the position back
//   node live-crash-test.mjs cleanup                                deletes "MDSEQ CRASH", checking its name
import { Bridge, DEFAULT_LIVE_PORT } from "file:///C:/dev/tuple/tools/live-mcp/bridge.mjs";

const [mode, a, b] = process.argv.slice(2);
const live = new Bridge({ name: "Live", port: DEFAULT_LIVE_PORT });
const get = (path, prop) => live.request("get", { path, prop });
const call = (path, method, args = []) => live.request("call", { path, method, args });
const set = (path, prop, value) => live.request("set", { path, prop, value });
const sleep = ms => new Promise(r => setTimeout(r, ms));
const names = async () => (await get("live_set", "tracks")).map(t => t.name);
const Name = "MDSEQ CRASH";

try
{
	if(mode === "setup")
	{
		if(await get("live_set", "is_playing"))
			throw new Error("Live is playing: nothing done");
		const before = await names();
		if(before.includes(Name))
			throw new Error(Name + " already in the set");
		await call("live_set", "create_midi_track", [before.length]);
		const t = `live_set tracks ${before.length}`;
		await set(t, "name", Name);
		if(await get(t, "name") !== Name)
			throw new Error("track not named " + Name);
		await set(`${t} mixer_device volume`, "value", 0);
		await set("live_set view", "selected_track", { __path__: t });
		await call("live_app browser", "load_item", [{ __path__: a }]);
		await sleep(5000);
		for(let i = 0; i < 2; ++i)
		{
			await call("live_app browser", "load_item", [{ __path__: b }]);
			await sleep(6000);
		}
		console.log("devices", (await get(t, "devices")).map(d => d.name).join(", "),
			"fader", await get(`${t} mixer_device volume`, "value"));
	}
	else if(mode === "run")
	{
		const seconds = Number(a || 20);
		const start = await get("live_set", "current_song_time");
		await set("live_set", "current_song_time", 0);
		console.log("start", new Date().toISOString());
		await call("live_set", "start_playing");
		const until = Date.now() + seconds * 1000;
		while(Date.now() < until)
		{
			await sleep(1000);
			console.log("playing", await get("live_set", "is_playing"), "beat",
				(await get("live_set", "current_song_time")).toFixed(2));
		}
		await call("live_set", "stop_playing");
		await set("live_set", "current_song_time", start);
		console.log("stopped", new Date().toISOString());
	}
	else if(mode === "cleanup")
	{
		const list = await names();
		const index = list.indexOf(Name);
		if(index >= 0 && list.filter(n => n === Name).length === 1)
			await call("live_set", "delete_track", [index]);
		console.log("tracks now:", (await names()).join(" | "));
	}
	else
		throw new Error("mode: setup, run or cleanup");
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
