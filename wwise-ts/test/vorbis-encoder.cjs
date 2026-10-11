const assert=require("node:assert/strict");
const {oggVorbisToWem,parseOggVorbis}=require("../dist/vorbis-encoder.js");
const {readWemFormat}=require("../dist/index.js");
const set32=(b,i,n)=>new DataView(b.buffer,b.byteOffset,b.byteLength).setUint32(i,n,true);
const set64=(b,i,n)=>new DataView(b.buffer,b.byteOffset,b.byteLength).setBigUint64(i,BigInt(n),true);
function oggPage(packet,seq,granule){
  if(packet.length>=255)throw Error("Test packets must be small");
  const b=new Uint8Array(28+packet.length);b.set(Buffer.from("OggS"));
  b[4]=0;b[5]=seq===0?2:seq===3?4:0;set64(b,6,granule);
  set32(b,14,8977);set32(b,18,seq);b[26]=1;b[27]=packet.length;b.set(packet,28);return b;
}
const ident=new Uint8Array(30);ident.set(Buffer.from([1,118,111,114,98,105,115]));ident[11]=2;set32(ident,12,48000);ident[28]=0xA8;ident[29]=1;
const comment=Buffer.from([3,118,111,114,98,105,115,0,0]);
const setup=Buffer.from([5,118,111,114,98,105,115,17,34,51]);
const audio=Buffer.from([0x13,0x88,0x42,0x10]);
const ogg=Buffer.concat([oggPage(ident,0,0),oggPage(comment,1,0),oggPage(setup,2,0),oggPage(audio,3,24000)]);
const parsed=parseOggVorbis(ogg);
assert.equal(parsed.channels,2);assert.equal(parsed.sampleRate,48000);
const wem=oggVorbisToWem(ogg);
assert.equal(readWemFormat(wem).codec,"Wwise Vorbis");
assert.equal(wem[4] + 256*wem[5],wem.length-8);
const vorbPos=Buffer.from(wem).indexOf(Buffer.from("vorb"));
assert.ok(vorbPos>0);
assert.equal(new DataView(wem.buffer).getUint32(vorbPos+8,true),24000);
assert.equal(new DataView(wem.buffer).getUint32(vorbPos+8+0x1c,true),9);
assert.equal(wem[vorbPos+8+0x30],8);
assert.equal(wem[vorbPos+8+0x31],10);
assert.throws(()=>parseOggVorbis(Buffer.from("not ogg")),/Ogg/);
process.stdout.write("Experimental Ogg-to-WEM container tests passed; vgmstream verification still required\n");
