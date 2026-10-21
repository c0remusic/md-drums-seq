// Issue 33's first test: can a Max device grab Push 2's eight track encoders (Track_Control_0..7) through the LOM, and
// what do they send? Logs to %TEMP%\mdseq-push-probe.txt. "start" (2 s after load) grabs them on the second control
// surface that lists them (Tuple's rule for the pad matrix); "grab n" (the "Probe Surface" parameter, which the LOM
// can set) grabs them on surface n instead; "release" gives them back, as does deleting the device.
autowatch = 0;
inlets = 1;
outlets = 1;

var LOG = "C:/Users/LEETJ/AppData/Local/Temp/mdseq-push-probe.txt";
var ready = false;
var surface = null;
var grabbed = [];
var observers = [];
var events = 0;

function noop() {}

function log(text)
{
	var f = new File(LOG, "readwrite");
	if(!f.isopen)
		f = new File(LOG, "write");
	if(!f.isopen)
		return;
	f.position = f.eof;
	f.writeline(Date.now() + " " + text);
	f.close();
}

function idOf(result)
{
	// LiveAPI.call returns ["id", n] for an object
	if(result instanceof Array)
		return Number(result[result.length - 1]);
	return Number(result);
}

function start()
{
	ready = true;
	log("start");
	var matches = [];
	for(var i = 0; i < 20; ++i)
	{
		var cs = new LiveAPI(noop, "control_surfaces " + i);
		if(!cs || Number(cs.id) === 0)
			continue;
		var names = cs.call("get_control_names");
		var text = names ? names.toString() : "";
		var has = text.indexOf("Track_Control_0") >= 0;
		log("surface " + i + " id " + cs.id + " type " + cs.type + " controls " + text.split(",").length + " encoders " +
			has);
		if(has)
			matches.push(i);
	}
	log("surfaces with encoders: " + matches.join(","));
	if(matches.length > 0)
		grabOn(matches.length > 1 ? matches[1] : matches[0]);
}

function grab(n)
{
	if(!ready)
		return;	// the parameter's value as the set restores it, before start
	grabOn(n);
}

function grabOn(index)
{
	release();
	surface = new LiveAPI(noop, "control_surfaces " + index);
	log("grabbing on surface " + index + " id " + surface.id);
	for(var e = 0; e < 8; ++e)
	{
		var id = idOf(surface.call("get_control", "Track_Control_" + e));
		log("Track_Control_" + e + " id " + id);
		if(!id)
			continue;
		surface.call("release_control", "id", id);
		surface.call("grab_control", "id", id);
		grabbed.push(id);
		observe(e, id);
	}
}

function observe(encoder, id)
{
	var o = new LiveAPI(function(args)
	{
		if(!args || args[0] !== "value")
			return;
		var v = Number(args[1]);
		var delta = v < 64 ? v : v - 128;
		++events;
		log("encoder " + encoder + " raw " + v + " delta " + delta + " events " + events);
		outlet(0, encoder, v);
	}, "id " + id);
	o.property = "value";
	observers.push(o);
}

function release()
{
	for(var i = 0; i < observers.length; ++i)
		observers[i].property = "";
	observers = [];
	if(surface)
		for(var g = 0; g < grabbed.length; ++g)
			surface.call("release_control", "id", grabbed[g]);
	if(grabbed.length)
		log("released " + grabbed.length);
	grabbed = [];
}

function notifydeleted()
{
	release();
}
