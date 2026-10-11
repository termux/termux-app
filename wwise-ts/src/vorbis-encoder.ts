/**
 * EXPERIMENTAL Wwise Vorbis WEM encoder: Ogg Vorbis -> legacy Wwise RIFF.
 *
 * Writes the explicit "vorb" chunk / size+granule audio packets layout.
 * This is NOT the stripped aoTuV-6.03 codebook / mod_packets format used by
 * many modern games. Test a generated WEM with vgmstream before installing.
 *
 * Does not encode PCM into Vorbis; supply a valid Ogg Vorbis bitstream first.
 */
type Packet={bytes:Uint8Array,granule:bigint};
function ascii(data:Uint8Array,at:number,len:number):string{
  return String.fromCharCode(...data.subarray(at,at+len));
}
function w32(arr:Uint8Array,at:number,n:number):void{
  new DataView(arr.buffer,arr.byteOffset,arr.byteLength).setUint32(at,n,true);
}
function r32(arr:Uint8Array,at:number):number{
  return new DataView(arr.buffer,arr.byteOffset,arr.byteLength).getUint32(at,true);
}
function concat(parts:Uint8Array[]):Uint8Array{
  const total=parts.reduce((n,p)=>n+p.length,0);
  const out=new Uint8Array(total);let at=0;
  for(const p of parts){out.set(p,at);at+=p.length;}
  return out;
}
export interface VorbisStream {channels:number;sampleRate:number;blockSmall:number;blockLarge:number;packets:readonly Packet[];}
export function parseOggVorbis(ogg:Uint8Array):VorbisStream{
  const packets:Packet[]=[];
  let at=0;
  let expectedSequence=0;
  let streamSerial:number|undefined;
  let current:Uint8Array[]=[];
  while(at<ogg.length){
    if(at+27>ogg.length || ascii(ogg,at,4)!=="OggS" || ogg[at+4]!==0)throw Error("Invalid Ogg page");
    const flags=ogg[at+5],segmentCount=ogg[at+26];
    const headerEnd=at+27+segmentCount;
    if(headerEnd>ogg.length)throw Error("Truncated Ogg segment table");
    let pageBytes=0;
    for(let i=at+27;i<headerEnd;i++)pageBytes+=ogg[i];
    if(headerEnd+pageBytes>ogg.length)throw Error("Truncated Ogg page");
    const serial=r32(ogg,at+14),sequence=r32(ogg,at+18);
    if(streamSerial===undefined){streamSerial=serial;expectedSequence=sequence;}
    if(serial!==streamSerial)throw Error("Chained/multiplexed Ogg unsupported");
    if(sequence!==expectedSequence++)throw Error("Out-of-order Ogg page");
    if(Boolean(flags&1)!==Boolean(current.length))throw Error("Invalid continued-packet flag");
    const dv=new DataView(ogg.buffer,ogg.byteOffset,ogg.byteLength);
    const granule=dv.getBigUint64(at+6,true);
    let pos=headerEnd;
    const complete:Packet[]=[];
    for(let i=at+27;i<headerEnd;i++){
      const length=ogg[i];
      current.push(ogg.subarray(pos,pos+length));pos+=length;
      if(length<255){
        complete.push({bytes:concat(current),granule:0n});
        current=[];
      }
    }
    if(complete.length)complete[complete.length-1].granule=granule;
    packets.push(...complete);
    at=pos;
  }
  if(current.length)throw Error("Truncated last Ogg packet");
  if(packets.length<4)throw Error("Ogg Vorbis needs headers and audio");
  const [id,comment,setup]=packets.map(p=>p.bytes);
  if(ascii(id,0,7)!=="\x01vorbis"||ascii(comment,0,7)!=="\x03vorbis"||ascii(setup,0,7)!=="\x05vorbis")throw Error("Not Vorbis headers");
  if(id.length<30||r32(id,7)!==0||id[29]!==1)throw Error("Invalid Vorbis identification");
  const channels=id[11],sampleRate=r32(id,12);
  const blockSmall=id[28]&15,blockLarge=id[28]>>4;
  if(!channels||!sampleRate||blockSmall<6||blockLarge>13||blockLarge<blockSmall)throw Error("Invalid Vorbis audio parameters");
  return {channels,sampleRate,blockSmall,blockLarge,packets};
}
function chunk(tag:string,payload:Uint8Array):Uint8Array{
  const out=new Uint8Array(8+payload.length+(payload.length%2));
  for(let i=0;i<4;i++)out[i]=tag.charCodeAt(i);
  w32(out,4,payload.length);out.set(payload,8);return out;
}
const u16=(a:Uint8Array,pos:number,value:number)=>{a[pos]=value&255;a[pos+1]=(value>>>8)&255;};
/**
 * Encodes the *legacy full-setup* Vorbis WEM variant, not modern codebook-stripped Wwise.
 * A correctly formed container does not guarantee vgmstream/game acceptance.
 */
export function oggVorbisToWem(ogg:Uint8Array):Uint8Array{
  const stream=parseOggVorbis(ogg);
  const {channels,sampleRate,blockSmall,blockLarge,packets}=stream;
  const lastGranule=packets.reduce((value,p)=>p.granule>value&&p.granule!==0xffffffffffffffffn?p.granule:value,0n);
  if(lastGranule>0xffffffffn)throw Error("Wwise legacy sample count exceeds 32 bits");
  const setup=packets[2].bytes.subarray(7);
  if(setup.length>65535)throw Error("Setup packet too large");
  const payloadParts:Uint8Array[]=[];
  const setupPacket=new Uint8Array(6+setup.length);
  u16(setupPacket,0,setup.length);setupPacket.set(setup,6);
  payloadParts.push(setupPacket);let dataLength=setupPacket.length;
  for(const packet of packets.slice(3)){
    if(packet.bytes.length>65535)throw Error("Audio packet exceeds Wwise 16-bit packet length");
    if(packet.granule>0xffffffffn&&packet.granule!==0xffffffffffffffffn)throw Error("Granule exceeds Wwise 32-bit range");
    const item=new Uint8Array(6+packet.bytes.length);
    u16(item,0,packet.bytes.length);
    w32(item,2,packet.granule===0xffffffffffffffffn?0:Number(packet.granule));
    item.set(packet.bytes,6);payloadParts.push(item);dataLength+=item.length;
  }
  const data=concat(payloadParts);
  const fmt=new Uint8Array(0x18);
  u16(fmt,0,0xffff);u16(fmt,2,channels);w32(fmt,4,sampleRate);
  w32(fmt,8,lastGranule?Math.floor(dataLength*sampleRate/Number(lastGranule)):0);
  u16(fmt,16,6);
  const vorb=new Uint8Array(0x34);
  w32(vorb,0,Number(lastGranule));
  w32(vorb,0x18,0);w32(vorb,0x1c,setupPacket.length);
  w32(vorb,0x2c,0xabcdef01);
  vorb[0x30]=blockSmall;vorb[0x31]=blockLarge;
  const parts=[chunk("fmt ",fmt),chunk("vorb",vorb),chunk("data",data)];
  const total=12+parts.reduce((n,p)=>n+p.length,0);
  const out=new Uint8Array(total);
  out.set([82,73,70,70],0);w32(out,4,total-8);
  out.set([87,65,86,69],8);
  let pos=12;for(const part of parts){out.set(part,pos);pos+=part.length;}
  return out;
}
