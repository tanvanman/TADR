#!/usr/bin/env python3
"""Derive caption-free Cycle button sprites from installed TA native art.

Only the Python standard library is used. The generated header embeds palette
indices, not screenshots or sampled game surfaces. Runtime captions remain TA
font text. Native GAF frame dimensions and borders are preserved exactly.

Usage:
  python make_cycle_button_gaf.py --escalation-archive /path/TAESC.gp3 \
      --prota-archive /path/ProTA.gp3 --stock-archive /path/totala1.hpi \
      --preview-directory /tmp/cycle-preview
"""
import argparse
import hashlib
import json
import struct
import zlib
from pathlib import Path

class Hpi:
    def __init__(self,path):
        self.path=Path(path)
        self.data=self.path.read_bytes()
        magic,version,size,key,start=struct.unpack_from('<5I',self.data)
        assert magic==0x49504148 and version==0x10000
        self.key=(~((key<<2)|(key>>6)))&255 if key else 0
        self.directory=self.read(0,size)
        self.files={}
        self.walk(start,'')
    def read(self,offset,size):
        b=self.data[offset:offset+size]
        if self.key:return bytes(v^255^((offset+i)&255)^self.key for i,v in enumerate(b))
        return b
    def walk(self,offset,prefix):
        count,entries=struct.unpack_from('<II',self.directory,offset)
        for i in range(count):
            name,record,folder=struct.unpack_from('<IIB',self.directory,entries+i*9)
            name=self.directory[name:self.directory.index(0,name)].decode('latin1')
            if folder:self.walk(record,prefix+name+'/')
            else:
                start,size,compressed=struct.unpack_from('<IIB',self.directory,record)
                self.files[(prefix+name).lower()]=(start,size,compressed)
    def extract(self,name):
        offset,size,compressed=self.files[name.lower()]
        if not compressed:return self.read(offset,size)
        chunks=(size+65535)//65536
        lengths=struct.unpack('<'+'I'*chunks,self.read(offset,chunks*4))
        offset+=4*chunks
        result=bytearray()
        for length in lengths:
            chunk=self.read(offset,length);offset+=length
            magic,unknown,method,encrypted,n,m,checksum=struct.unpack_from('<4sBBBIII',chunk)
            assert magic==b'SQSH'
            payload=chunk[19:19+n]
            if encrypted:payload=bytes(((v-i)&255)^i&255 for i,v in enumerate(payload))
            if method==2:part=zlib.decompress(payload)
            elif method==0:part=payload
            else:raise ValueError((name,method))
            assert len(part)==m
            result.extend(part)
        assert len(result)==size
        return bytes(result)

def gaf_entries(data):
    magic,count,reserved=struct.unpack_from('<III',data)
    assert magic==0x10100
    entries={}
    for pointer in struct.unpack_from('<'+'I'*count,data,12):
        frames=struct.unpack_from('<H',data,pointer)[0]
        name=data[pointer+8:pointer+40].split(b'\0')[0].decode('latin1')
        entries[name]=[struct.unpack_from('<II',data,pointer+40+i*8)[0] for i in range(frames)]
    return entries

def gaf_frame(data,pointer):
    width,height,x,y,background,compressed,subframes,unknown,pixels,extra=struct.unpack_from('<HHhhBBHIII',data,pointer)
    assert not subframes
    output=bytearray([background])*(width*height)
    pos=pixels
    for row in range(height):
        if not compressed:
            output[row*width:(row+1)*width]=data[pos:pos+width];pos+=width
            continue
        length=struct.unpack_from('<H',data,pos)[0];pos+=2
        end=pos+length;col=0
        while pos<end:
            control=data[pos];pos+=1
            if control&1:col+=control>>1
            elif control&2:
                n=(control>>2)+1
                output[row*width+col:row*width+col+n]=bytes([data[pos]])*n
                pos+=1;col+=n
            else:
                n=(control>>2)+1
                output[row*width+col:row*width+col+n]=data[pos:pos+n]
                pos+=n;col+=n
        assert col<=width
    return width,height,bytes(output),background

def first_face(data,pointer):
    """The first child of composite native buttons is their uncaptioned face."""
    values=struct.unpack_from('<HHhhBBHIII',data,pointer)
    if values[6]:
        pointer=struct.unpack_from('<I',data,values[8])[0]
    return gaf_frame(data,pointer)

def derive(escalation,prota):
    sources=[]
    sets=[]
    for archive in (escalation,prota):
        data=archive.extract('anims/commongui.gaf')
        entries=gaf_entries(data)
        sources.append({'archive':archive.path.name,'entry':'anims/commongui.gaf',
                        'sha256':hashlib.sha256(data).hexdigest()})
        faction=[]
        for name,width in (('ARMONOFF',113),('CORONOFF',114)):
            frames=[]
            # Native DrawButton uses the last two source frames for pressed
            # and disabled. Five-frame ONOFF sets also contain a help frame
            # at index 2; it must not become the four-frame Cycle press art.
            source_frames=entries[name]
            indices=(0,1,len(source_frames)-2,len(source_frames)-1)
            for index in indices:
                pointer=source_frames[index]
                w,h,pixels,background=first_face(data,pointer)
                assert (w,h)==(width,21)
                assert 255 not in pixels
                frames.append(bytearray(pixels))
            assert len(frames)==4
            # Escalation's CORE sequence is the original flat GAF: captions
            # were baked into frames 0..2. Its own disabled frame 3 supplies
            # the exact native background, including its middle divider.
            # Keep every outer-border/indicator pixel of the source state.
            if archive is escalation and name=='CORONOFF':
                for frame in frames[:3]:
                    for row in range(3,20):
                        frame[row*width+4:row*width+101]=frames[3][row*width+4:row*width+101]
            faction.append([bytes(frame) for frame in frames])
        sets.append(faction)
    # Both installed mods use identical ARM face pixels, despite repacking
    # their GAFs differently. ProTA intentionally changes CORE's face style.
    assert sets[0][0]==sets[1][0]
    return sets[0][0],sets[0][1],sets[1][1],sources

def array(name,frames,width):
    lines=['    constexpr unsigned char '+name+'[Frames]['+str(width)+' * Height] = {']
    for index,frame in enumerate(frames):
        lines.append('        { // '+('Cycle Off','Cycle On','Pressed','Disabled')[index])
        for offset in range(0,len(frame),28):
            lines.append('            '+','.join(str(v) for v in frame[offset:offset+28])+',')
        lines.append('        },')
    lines.append('    };')
    return '\n'.join(lines)

def header(arm,core_esc,core_prota,sources):
    return '''#pragma once

// Generated by tools/make_cycle_button_gaf.py from native TA button GAFs.
// Caption-free faces retain original palette indices, borders and lamps.
// ARMONOFF uses its first composite child. CORE follows the selected mod:
// ESC's original blue frame, ProTA's metallic replacement. No game screen
// is sampled and no original ON/OFF caption is retained in these sprites.
// Source commongui.gaf SHA256:
// ESC: '''+sources[0]['sha256']+'''
// ProTA: '''+sources[1]['sha256']+'''
namespace LabBuildCycleArt
{
    constexpr unsigned ArmWidth = 113;
    constexpr unsigned CoreWidth = 114;
    constexpr unsigned Height = 21;
    constexpr unsigned Frames = 4;
    constexpr unsigned char Transparent = 255;

'''+array('Arm',arm,113)+'''

#if defined(TDRAW_CONFIG_PROTA)
'''+array('Core',core_prota,114)+'''
#else
'''+array('Core',core_esc,114)+'''
#endif
}
'''

def png(path,width,height,rgb):
    def chunk(kind,data):
        return struct.pack('>I',len(data))+kind+data+struct.pack('>I',zlib.crc32(kind+data)&0xffffffff)
    rows=b''.join(b'\0'+bytes(rgb[row*width*3:(row+1)*width*3]) for row in range(height))
    path.write_bytes(b'\x89PNG\r\n\x1a\n'+chunk(b'IHDR',struct.pack('>IIBBBBB',width,height,8,2,0,0,0))
                     +chunk(b'IDAT',zlib.compress(rows,9))+chunk(b'IEND',b''))

def caption(frame,width,font,text,colour=83):
    """Use TA's own bit-packed console font for an illustrative label."""
    glyphs=[]
    for c in text.encode('ascii'):
        off=struct.unpack_from('<H',font,4+2*(c-font[3]))[0]
        if off:glyphs.append((font[off],font[off+1:]))
    total=sum(w for w,_ in glyphs)
    x=(width-16-total)//2
    y=(21-font[0])//2
    result=bytearray(frame)
    for w,bits in glyphs:
        bit=0
        for row in range(font[0]):
            for col in range(w):
                if bits[bit//8]&(128>>(bit%8)):
                    result[(y+row)*width+x+col]=colour
                bit+=1
        x+=w
    return bytes(result)

def preview(directory,arm,core_esc,core_prota,palette,font,sources):
    directory.mkdir(parents=True,exist_ok=True)
    colours=[palette[i*4:i*4+3] for i in range(256)]
    for profile,core in (('escalation',core_esc),('prota',core_prota)):
        # Two columns (ARM, CORE), four rows (off/on/pressed/disabled).
        # Every sprite is enlarged by integer nearest-neighbour replication;
        # only the added TA-font caption differs from the production art.
        scale=4;canvas_width=980;canvas_height=400
        rgb=bytearray(bytes((28,28,28))*(canvas_width*canvas_height))
        for col,(frames,w) in enumerate(((arm,113),(core,114))):
            for row,frame in enumerate(frames):
                face=caption(frame,w,font,('CYCLE OFF','CYCLE ON','CYCLE ON','CYCLE OFF')[row],83 if row!=3 else 90)
                png(directory/(profile+'-'+('arm','core')[col]+'-'+str(row)+'.png'),w,21,
                    b''.join(colours[p] for p in face))
                for y in range(21):
                    for x in range(w):
                        colour=colours[face[y*w+x]]
                        for dy in range(scale):
                            start=((row*98+7+y*scale+dy)*canvas_width+col*490+10+x*scale)*3
                            rgb[start:start+3*scale]=colour*scale
        png(directory/(profile+'-cycle-buttons.png'),canvas_width,canvas_height,rgb)
    (directory/'provenance.json').write_text(json.dumps({'sources':sources,'states':['Off','On','Pressed','Disabled'],
        'caption_preview':'Actual TA console.fnt glyphs; runtime uses its active native button font',
        'pixel_dimensions':{'arm':[113,21],'core':[114,21]},'game_screenshots_modified':False},indent=2)+'\n')

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--escalation-archive',type=Path,required=True)
    parser.add_argument('--prota-archive',type=Path,required=True)
    parser.add_argument('--stock-archive',type=Path,required=True)
    parser.add_argument('--output-header',type=Path,default=Path(__file__).resolve().parents[1]/'LabBuildCycleArt.h')
    parser.add_argument('--preview-directory',type=Path)
    parser.add_argument('--check',action='store_true')
    args=parser.parse_args()
    esc,prota,stock=(Hpi(path) for path in (args.escalation_archive,args.prota_archive,args.stock_archive))
    arm,core_esc,core_prota,sources=derive(esc,prota)
    generated=header(arm,core_esc,core_prota,sources)
    if args.check:
        assert args.output_header.read_text()==generated,'Generated native art differs'
    else:args.output_header.write_text(generated)
    if args.preview_directory:
        palette=stock.extract('palettes/palette.pal')
        font=esc.extract('fonts/console.fnt')
        preview(args.preview_directory,arm,core_esc,core_prota,palette,font,sources)
    print('Native Cycle sprites verified' if args.check else 'Generated native Cycle sprites')

if __name__=='__main__':main()
