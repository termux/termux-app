/** Small Wwise soundbank format toolkit, NOT a Wwise Vorbis encoder. */
const ascii=(bytes: Uint8Array,off:number,size=4):string => String.fromCharCode(...bytes.subarray(off,off+size));
const u32=(bytes:Uint8Array,off:number):number => new DataView(bytes.buffer,bytes.byteOffset,bytes.byteLength).getUint32(off,true);
const u16=(bytes:Uint8Array,off:number):number => new DataView(bytes.buffer,bytes.byteOffset,bytes.byteLength).getUint16(off,true);
const set32=(bytes:Uint8Array,off:number,value:number):void => new DataView(bytes.buffer,bytes.byteOffset,bytes.byteLength).setUint32(off,value,true);
export type CodecName="Wwise Vorbis"|"PCM"|"IMA ADPCM"|"Unknown";
export interface WemFormat { codecTag:number; codec:CodecName; channels:number; sampleRate:number; bitsPerSample:number; }
export interface AudioEntry { id:number; offset:number; size:number; }
export interface BnkChunk { tag:string; payload:Uint8Array; }
export function readWemFormat(wem:Uint8Array):WemFormat {
  if(wem.length<20 || ascii(wem,0)!=="RIFF" || ascii(wem,8)!=="WAVE") throw Error("Not a WEM RIFF/WAVE");
  let offset=12;
  while(offset+8<=wem.length){
    const tag=ascii(wem,offset), size=u32(wem,offset+4);
    if(size>wem.length-offset-8) throw Error("Corrupt WEM chunk");
    if(tag==="fmt "){
      if(size<16) throw Error("Invalid WEM fmt");
      const p=offset+8, codecTag=u16(wem,p);
      const codec:CodecName=codecTag===0xffff?"Wwise Vorbis":
        codecTag===1||codecTag===0xfffe?"PCM":
        codecTag===2||codecTag===0x69?"IMA ADPCM":"Unknown";
      return {codecTag,codec,channels:u16(wem,p+2),sampleRate:u32(wem,p+4),bitsPerSample:u16(wem,p+14)};
    }
    offset+=8+size+(size%2);
  }
  throw Error("Missing WEM fmt chunk");
}
export class SoundBank {
  readonly chunks:BnkChunk[]=[];
  readonly entries:AudioEntry[]=[];
  private trailing:Uint8Array=new Uint8Array();
  constructor(readonly bytes:Uint8Array){
    if(bytes.length<8||ascii(bytes,0)!=="BKHD")throw Error("Not a Wwise soundbank");
    let off=0;
    while(off+8<=bytes.length){
      const tag=ascii(bytes,off),size=u32(bytes,off+4);
      if(size>bytes.length-off-8)throw Error("Invalid BNK section length");
      this.chunks.push({tag,payload:bytes.slice(off+8,off+8+size)});
      off+=8+size;
    }
    this.trailing=bytes.slice(off);
    const index=this.chunks.find(c=>c.tag==="DIDX")?.payload;
    const data=this.chunks.find(c=>c.tag==="DATA")?.payload;
    if(!index||!data||index.length%12!==0)throw Error("BNK requires valid DIDX and DATA");
    const used: Array<[number,number]>=[];
    for(let i=0;i<index.length;i+=12){
      const id=u32(index,i),start=u32(index,i+4),size=u32(index,i+8);
      if(start>data.length||size>data.length-start)throw Error("WEM outside DATA");
      if(used.some(([a,b])=>start<b&&start+size>a))throw Error("Overlapping WEM entries");
      if(this.entries.some(e=>e.id===id))throw Error("Duplicate WEM ID");
      used.push([start,start+size]);this.entries.push({id,offset:start,size});
    }
  }
  extract(id:number):Uint8Array{
    const entry=this.entries.find(e=>e.id===id);
    const data=this.chunks.find(c=>c.tag==="DATA")!.payload;
    if(!entry)throw Error("Audio ID not found: "+id);
    return data.slice(entry.offset,entry.offset+entry.size);
  }
  repack(replacements:ReadonlyMap<number,Uint8Array>,options:{allowCodecChange?:boolean}={}):Uint8Array{
    if(!replacements.size)return this.bytes.slice();
    for(const [id,wem] of replacements){
      if(!this.entries.some(e=>e.id===id))throw Error("Unknown WEM "+id);
      const before=readWemFormat(this.extract(id)),after=readWemFormat(wem);
      if(!options.allowCodecChange&&before.codecTag!==after.codecTag)throw Error(
        "Codec mismatch on "+id+": "+before.codec+" -> "+after.codec+". A matching Wwise encoder is required.");
    }
    const index=new Uint8Array(this.entries.length*12);
    const parts:Uint8Array[]=[];let length=0;
    for(const [i,e] of this.entries.entries()){
      const media=replacements.get(e.id)??this.extract(e.id);
      const padding=(16-length%16)%16;
      if(padding){parts.push(new Uint8Array(padding));length+=padding;}
      set32(index,i*12,e.id);set32(index,i*12+4,length);set32(index,i*12+8,media.length);
      parts.push(media);length+=media.length;
    }
    const outData=new Uint8Array(length);let off=0;
    for(const part of parts){outData.set(part,off);off+=part.length;}
    const sections=this.chunks.map(c=>({...c,payload:c.tag==="DIDX"?index:c.tag==="DATA"?outData:c.payload}));
    const total=sections.reduce((n,c)=>n+8+c.payload.length,0)+this.trailing.length;
    const output=new Uint8Array(total);off=0;
    for(const c of sections){
      for(let j=0;j<4;j++)output[off+j]=c.tag.charCodeAt(j);
      set32(output,off+4,c.payload.length);off+=8;
      output.set(c.payload,off);off+=c.payload.length;
    }
    output.set(this.trailing,off);
    const verify=new SoundBank(output);
    for(const [id,wem] of replacements){
      const read=verify.extract(id);
      if(read.length!==wem.length||read.some((b,i)=>b!==wem[i]))throw Error("Repacked WEM verification failed");
    }
    return output;
  }
}
