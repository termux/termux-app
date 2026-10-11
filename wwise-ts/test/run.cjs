const assert=require("node:assert/strict");
const {SoundBank,readWemFormat}=require("../dist/index.js");
const u32=(b,o,v)=>new DataView(b.buffer,b.byteOffset,b.byteLength).setUint32(o,v,true);
function chunk(tag,payload){const b=new Uint8Array(8+payload.length);b.set(Buffer.from(tag));u32(b,4,payload.length);b.set(payload,8);return b}
function join(...parts){return Buffer.concat(parts.map(Buffer.from))}
function wem(codec,nonce){const b=new Uint8Array(44);b.set(Buffer.from("RIFF"));u32(b,4,36);b.set(Buffer.from("WAVEfmt "),8);u32(b,16,16);b[20]=codec&255;b[21]=codec>>>8;b[22]=2;u32(b,24,48000);b[34]=16;b.set(Buffer.from("data"),36);b[43]=nonce;return b}
const a=wem(0xffff,1),b=wem(0xffff,2);
const idx=new Uint8Array(24);u32(idx,0,101);u32(idx,4,0);u32(idx,8,44);u32(idx,12,202);u32(idx,16,48);u32(idx,20,44);
const data=new Uint8Array(92);data.set(a);data.set(b,48);
const original=join(chunk("BKHD",new Uint8Array(28)),chunk("DIDX",idx),chunk("DATA",data),chunk("HIRC",new Uint8Array([5,4,3])));
const bank=new SoundBank(original);assert.equal(bank.entries.length,2);assert.equal(readWemFormat(bank.extract(101)).codec,"Wwise Vorbis");
const updated=wem(0xffff,9),result=new SoundBank(bank.repack(new Map([[202,updated]])));
assert.deepEqual(result.extract(101),a);assert.deepEqual(result.extract(202),updated);
assert.deepEqual(result.chunks.find(c=>c.tag==="HIRC").payload,new Uint8Array([5,4,3]));
assert.throws(()=>bank.repack(new Map([[202,wem(1,8)]])),/Codec mismatch/);
assert.throws(()=>bank.repack(new Map([[303,updated]])),/Unknown WEM/);
assert.deepEqual(bank.repack(new Map()),original);
process.stdout.write("Wwise BNK/WEM TypeScript tests passed\n");
