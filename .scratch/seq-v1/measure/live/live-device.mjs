// Read-only: the parameters of MD Drums Seq on a track, and the track's clips, through the TupleMCP bridge.
//   node live-device.mjs <track index> <device index>
import { Bridge, DEFAULT_LIVE_PORT } from "file:///C:/dev/tuple/tools/live-mcp/bridge.mjs";

const [trackIndex, deviceIndex] = process.argv.slice(2).map(Number);
const live = new Bridge({ name: "Live", port: DEFAULT_LIVE_PORT });
const get = (path, prop) => live.request("get", prop ? { path, prop } : { path });
const getMany = (path, props) => live.request("get", { path, props });

try
{
	const device = `live_set tracks ${trackIndex} devices ${deviceIndex}`;
	console.log("device", JSON.stringify(await getMany(device, ["name", "class_name", "is_active"])));
	const params = await get(device, "parameters");
	for(let i = 0; i < (params?.length ?? 0); ++i)
	{
		const p = await getMany(`${device} parameters ${i}`, ["name", "value", "min", "max", "is_enabled", "state"]);
		console.log(i, JSON.stringify(p.props ?? p));
	}
	const track = `live_set tracks ${trackIndex}`;
	const arr = await get(track, "arrangement_clips");
	console.log("arrangement clips", Array.isArray(arr) ? arr.length : JSON.stringify(arr));
	const slots = await get(track, "clip_slots");
	let withClip = 0;
	for(let s = 0; s < (slots?.length ?? 0); ++s)
		if(await get(`${track} clip_slots ${s}`, "has_clip"))
			++withClip;
	console.log("session clips", withClip, "of", slots?.length);
	console.log("playing_slot_index", await get(track, "playing_slot_index"));
	console.log("meter", JSON.stringify(await getMany(track, ["output_meter_level", "output_meter_left", "output_meter_right"])));
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
