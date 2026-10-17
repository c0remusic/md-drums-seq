// Read-only look at the open Live set through the TupleMCP bridge.
import { Bridge, DEFAULT_LIVE_PORT } from "file:///C:/dev/tuple/tools/live-mcp/bridge.mjs";

const live = new Bridge({ name: "Live", port: DEFAULT_LIVE_PORT });
const get = (path, prop) => live.request("get", prop ? { path, prop } : { path });
const getMany = (path, props) => live.request("get", { path, props });

try
{
	const song = await getMany("live_set", ["tempo", "is_playing", "current_song_time", "record_mode"]);
	console.log("song", JSON.stringify(song));
	const tracks = await get("live_set", "tracks");
	console.log("tracks raw", JSON.stringify(tracks).slice(0, 300));
	const count = Array.isArray(tracks) ? tracks.length : (tracks?.length ?? 0);
	for(let i = 0; i < count; ++i)
	{
		const t = await getMany(`live_set tracks ${i}`, ["name", "has_midi_input", "has_audio_input", "arm", "mute"]);
		const devices = await get(`live_set tracks ${i}`, "devices");
		const names = [];
		for(let d = 0; d < (devices?.length ?? 0); ++d)
			names.push(await get(`live_set tracks ${i} devices ${d}`, "name"));
		console.log(i, JSON.stringify(t.props ?? t), names.join(" | "));
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
