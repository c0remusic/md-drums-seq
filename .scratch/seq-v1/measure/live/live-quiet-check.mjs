// Read only: whether starting the transport would sound anything but the given track. Lists, for every other track,
// its Arrangement clips, the Session clips playing or triggered, its output routing, mute and solo; and the master.
//   node live-quiet-check.mjs <track name kept>
import { Bridge, DEFAULT_LIVE_PORT } from "file:///C:/dev/tuple/tools/live-mcp/bridge.mjs";

const kept = process.argv[2];
const live = new Bridge({ name: "Live", port: DEFAULT_LIVE_PORT });
const get = (path, prop) => live.request("get", { path, prop });

try
{
	const tracks = await get("live_set", "tracks");
	for(let i = 0; i < (tracks?.length ?? 0); ++i)
	{
		const path = `live_set tracks ${i}`;
		const name = await get(path, "name");
		const arrangement = await get(path, "arrangement_clips");
		const slots = await get(path, "clip_slots");
		const playing = [];
		for(let s = 0; s < (slots?.length ?? 0); ++s)
		{
			const has = await get(`${path} clip_slots ${s}`, "has_clip");
			if(!has)
				continue;
			const p = await get(`${path} clip_slots ${s} clip`, "is_playing");
			const t = await get(`${path} clip_slots ${s} clip`, "is_triggered");
			if(p || t)
				playing.push(s);
		}
		const mute = await get(path, "mute");
		console.log(i, JSON.stringify(name), name === kept ? "(kept)" : "", "arrangement clips",
			arrangement?.length ?? 0, "session playing", JSON.stringify(playing), "mute", mute);
	}
	console.log("song time", await get("live_set", "current_song_time"), "loop", await get("live_set", "loop"),
		"loop start", await get("live_set", "loop_start"), "length", await get("live_set", "loop_length"));
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
