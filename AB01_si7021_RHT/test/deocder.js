function decodeUplink(input) {
    var data = {};
    // var fport = {
      // 1: "setup",
      // 2: "interval",
      // 3: "motion",
      // 4: "button"
    // };
    // data.event = events[input.fPort];
    t = 0;
    h = 4;
    data.fPort = input.fPort
    data.temperature = bytesToFloat(input.bytes.slice(t,t+=4));
    data.humidity = bytesToFloat(input.bytes.slice(h,h+=4));
    data.battery = ((input.bytes[8] << 8) + input.bytes[9])/1000.;
    var warnings = [];
    // if (data.temperature < -10) {
      // warnings.push("it's cold");
    // }
    return {
      data: data //,
      // warnings: warnings
    };
  }
  
  function bytesToInt(by) {
    f = by[0] | by[1]<<8 | by[2]<<16 | by[3]<<24;
    return f;
  } 
  
  function bytesToFloat(by) {
    var bits = by[3]<<24 | by[2]<<16 | by[1]<<8 | by[0];
    var sign = (bits>>>31 === 0) ? 1.0 : -1.0;
    var e = bits>>>23 & 0xff;
    var m = (e === 0) ? (bits & 0x7fffff)<<1 : (bits & 0x7fffff) | 0x800000;
    var f = sign * m * Math.pow(2, e - 150);
    return f;
  } 
