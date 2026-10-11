// Generates the APK asset from TypeScript output. Never edit the generated asset by hand.
const fs=require("node:fs");
const path=require("node:path");
const input=path.join(__dirname,"..","dist","vorbis-encoder.js");
const target=path.join(__dirname,"..","..","app","src","main","assets","wwise-legacy-vorbis.js");
if(!fs.existsSync(input))throw Error("Compile wwise-ts first");
fs.mkdirSync(path.dirname(target),{recursive:true});
fs.copyFileSync(input,target);
console.log("Bundled compiled TypeScript encoder in Android assets");
