// Read-only: walks a browser path by names and prints the children found at each level.
//   node live-browse.mjs "live_app browser midi_effects" "Max MIDI Effect" "md-drums-seq"
import { Bridge, DEFAULT_LIVE_PORT } from "file:///C:/dev/tuple/tools/live-mcp/bridge.mjs";

const [root, ...names] = process.argv.slice(2);
const live = new Bridge({ name: "Live", port: DEFAULT_LIVE_PORT });
const get = (path, prop) => live.request("get", { path, prop });

try
{
	let path = root;
	for(let level = 0; ; ++level)
	{
		const children = await get(path, "children");
		const list = Array.isArray(children) ? children : [];
		console.log(path, "->", list.map((c, i) => i + ":" + c.name).join(" | ").slice(0, 2000));
		if(level >= names.length)
			break;
		const index = list.findIndex(c => c.name === names[level]);
		if(index < 0)
		{
			console.log("not found:", names[level]);
			break;
		}
		path = `${path} children ${index}`;
	}
	console.log("final path:", path);
	const item = await live.request("get", { path, props: ["name", "is_loadable", "is_device", "uri"] });
	console.log(JSON.stringify(item));
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
