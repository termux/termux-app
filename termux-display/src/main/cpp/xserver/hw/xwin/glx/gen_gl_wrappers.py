#!/usr/bin/python3
#
# python script to generate cdecl to stdcall wrappers for GL functions
# adapted from genheaders.py
#
# Copyright (c) 2013 The Khronos Group Inc.
#
# Permission is hereby granted, free of charge, to any person obtaining a
# copy of this software and/or associated documentation files (the
# "Materials"), to deal in the Materials without restriction, including
# without limitation the rights to use, copy, modify, merge, publish,
# distribute, sublicense, and/or sell copies of the Materials, and to
# permit persons to whom the Materials are furnished to do so, subject to
# the following conditions:
#
# The above copyright notice and this permission notice shall be included
# in all copies or substantial portions of the Materials.
#
# THE MATERIALS ARE PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
# EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
# MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
# IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
# CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
# TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
# MATERIALS OR THE USE OR OTHER DEALINGS IN THE MATERIALS.

import sys, time, pdb, string, cProfile
from reg import *

# Default input / log files
errFilename = None
diagFilename = 'diag.txt'
regFilename = 'gl.xml'
outFilename = 'gen_gl_wrappers.c'

protect=True
prefix="gl"
preresolve=False
wrapper=False
shim=False
thunk=False
thunkdefs=False
staticwrappers=False
nodebug=False

# list of WGL extension functions we use
used_wgl_ext_fns = {key: 1 for key in [
    "wglSwapIntervalEXT",
    "wglGetExtensionsStringARB",
    "wglDestroyPbufferARB",
    "wglGetPbufferDCARB",
    "wglReleasePbufferDCARB",
    "wglCreatePbufferARB",
    "wglMakeContextCurrentARB",
    "wglChoosePixelFormatARB",
    "wglGetPixelFormatAttribivARB",
    "wglGetPixelFormatAttribivARB"
]}

if __name__ == '__main__':
    i = 1
    while (i < len(sys.argv)):
        arg = sys.argv[i]
        i = i + 1
        if (arg == '-noprotect'):
            print('Disabling inclusion protection in output headers', file=sys.stderr)
            protect = False
        elif (arg == '-registry'):
            regFilename = sys.argv[i]
            i = i+1
            print('Using registry', regFilename, file=sys.stderr)
        elif (arg == '-outfile'):
            outFilename = sys.argv[i]
            i = i+1
        elif (arg == '-preresolve'):
            preresolve=True
        elif (arg == '-wrapper'):
            wrapper=True
        elif (arg == '-shim'):
            shim=True
        elif (arg == '-thunk'):
            thunk=True
        elif (arg == '-thunkdefs'):
            thunkdefs=True
        elif (arg == '-staticwrappers'):
            staticwrappers=True
        elif (arg == '-prefix'):
            prefix = sys.argv[i]
            i = i+1
        elif (arg == '-nodebug'):
            nodebug = True
        elif (arg[0:1] == '-'):
            print('Unrecognized argument:', arg, file=sys.stderr)
            exit(1)

print('Generating', outFilename, file=sys.stderr)

# Load & parse registry
reg = Registry()
tree = etree.parse(regFilename)
reg.loadElementTree(tree)

if shim:
    versions = '1\.[012]'
else:
    versions = '.*'

genOpts = CGeneratorOptions(
        apiname           = prefix,
        profile           = 'compatibility',
        versions          = versions,
        emitversions      = versions,
        defaultExtensions = prefix,                   # Default extensions for GL
        protectFile       = protect,
        protectFeature    = protect,
        protectProto      = protect,
        )

# create error/warning & diagnostic files
if (errFilename):
    errWarn = open(errFilename,'w')
else:
    errWarn = sys.stderr
diag = open(diagFilename, 'w')

def ParseCmdRettype(cmd):
    proto=noneStr(cmd.elem.find('proto'))
    rettype=noneStr(proto.text)
    if rettype.lower()!="void ":
        plist = ([t for t in proto.itertext()])
        rettype = ''.join(plist[:-1])
    rettype=rettype.strip()
    return rettype

def ParseCmdParams(cmd):
    params = cmd.elem.findall('param')
    plist=[]
    for param in params:
        # construct the formal parameter definition from ptype and name
        # elements, also using any text found around these in the
        # param element, in the order it appears in the document
        paramtype = ''
        # also extract the formal parameter name from the name element
        paramname = ''
        for t in param.iter():
            if t.tag == 'ptype' or t.tag == 'param':
                paramtype = paramtype + noneStr(t.text)
            if t.tag == 'name':
                paramname = t.text + '_'
                paramtype = paramtype + ' ' + paramname
            if t.tail is not None:
                paramtype = paramtype + t.tail.strip()
        plist.append((paramtype, paramname))
    return plist

class PreResolveOutputGenerator(OutputGenerator):
    def __init__(self,
                 errFile = sys.stderr,
                 warnFile = sys.stderr,
                 diagFile = sys.stdout):
        OutputGenerator.__init__(self, errFile, warnFile, diagFile)
        self.wrappers={}
    def beginFile(self, genOpts):
        self.outFile.write('/* Automatically generated from %s - DO NOT EDIT */\n\n'%regFilename)
    def endFile(self):
        self.outFile.write('\nvoid ' + prefix + 'ResolveExtensionProcs(void)\n{\n')
        for funcname in self.wrappers.keys():
            self.outFile.write( '  PRERESOLVE(PFN' + funcname.upper() + 'PROC, "' + funcname + '");\n')
        self.outFile.write('}\n\n')
    def beginFeature(self, interface, emit):
        OutputGenerator.beginFeature(self, interface, emit)
    def endFeature(self):
        OutputGenerator.endFeature(self)
    def genType(self, typeinfo, name):
        OutputGenerator.genType(self, typeinfo, name)
    def genEnum(self, enuminfo, name):
        OutputGenerator.genEnum(self, enuminfo, name)
    def genCmd(self, cmd, name):
        OutputGenerator.genCmd(self, cmd, name)

        if prefix == 'wgl' and not name in used_wgl_ext_fns:
            return

        self.outFile.write('RESOLVE_DECL(PFN' + name.upper() + 'PROC);\n')
        self.wrappers[name]=1

class WrapperOutputGenerator(OutputGenerator):
    def __init__(self,
                 errFile = sys.stderr,
                 warnFile = sys.stderr,
                 diagFile = sys.stdout):
        OutputGenerator.__init__(self, errFile, warnFile, diagFile)
    def beginFile(self, genOpts):
        self.outFile.write('/* Automatically generated from %s - DO NOT EDIT */\n\n'%regFilename)
    def endFile(self):
        pass
    def beginFeature(self, interface, emit):
        OutputGenerator.beginFeature(self, interface, emit)
        self.OldVersion = self.featureName.startswith('GL_VERSION_1_0') or self.featureName.startswith('GL_VERSION_1_1')
    def endFeature(self):
        OutputGenerator.endFeature(self)
    def genType(self, typeinfo, name):
        OutputGenerator.genType(self, typeinfo, name)
    def genEnum(self, enuminfo, name):
        OutputGenerator.genEnum(self, enuminfo, name)
    def genCmd(self, cmd, name):
        OutputGenerator.genCmd(self, cmd, name)

        if prefix == 'wgl' and not name in used_wgl_ext_fns:
            return

        rettype=ParseCmdRettype(cmd)

        if staticwrappers: self.outFile.write("static ")
        self.outFile.write("%s %sWrapper("%(rettype, name))
        plist=ParseCmdParams(cmd)
        Comma=""
        if len(plist):
            for ptype, pname in plist:
                self.outFile.write("%s%s"%(Comma, ptype))
                Comma=", "
        else:
            self.outFile.write("void")

        self.outFile.write(")\n{\n")

        # for GL 1.0 and 1.1 functions, generate stdcall wrappers which call the function directly
        if self.OldVersion:
            if not nodebug:
                self.outFile.write('  if (glxWinDebugSettings.enable%scallTrace) ErrorF("%s\\n");\n'%(prefix.upper(), name))
                self.outFile.write("  glWinDirectProcCalls++;\n")
                self.outFile.write("\n")

            if rettype.lower()=="void":
                self.outFile.write("  %s( "%(name))
            else:
                self.outFile.write("  return %s( "%(name))

            Comma=""
            for ptype, pname in plist:
                self.outFile.write("%s%s"%(Comma, pname))
                Comma=", "

        # for GL 1.2+ functions, generate stdcall wrappers which use wglGetProcAddress()
        else:
            if rettype.lower()=="void":
                self.outFile.write('  RESOLVE(PFN%sPROC, "%s");\n'%(name.upper(), name))

                if not nodebug:
                    self.outFile.write("\n")
                    self.outFile.write('  if (glxWinDebugSettings.enable%scallTrace) ErrorF("%s\\n");\n'%(prefix.upper(), name))
                    self.outFile.write("\n")

                self.outFile.write("  RESOLVED_PROC(PFN%sPROC)( """%(name.upper()))
            else:
                self.outFile.write('  RESOLVE_RET(PFN%sPROC, "%s", FALSE);\n'%(name.upper(), name))

                if not nodebug:
                    self.outFile.write("\n")
                    self.outFile.write('  if (glxWinDebugSettings.enable%scallTrace) ErrorF("%s\\n");\n'%(prefix.upper(), name))
                    self.outFile.write("\n")

                self.outFile.write("  return RESOLVED_PROC(PFN%sPROC)("%(name.upper()))

            Comma=""
            for ptype, pname in plist:
                self.outFile.write("%s%s"%(Comma, pname))
                Comma=", "
        self.outFile.write(" );\n}\n\n")

class ThunkOutputGenerator(OutputGenerator):
    def __init__(self,
                 errFile = sys.stderr,
                 warnFile = sys.stderr,
                 diagFile = sys.stdout):
        OutputGenerator.__init__(self, errFile, warnFile, diagFile)
    def beginFile(self, genOpts):
        self.outFile.write('/* Automatically generated from %s - DO NOT EDIT */\n\n'%regFilename)
    def endFile(self):
        pass
    def beginFeature(self, interface, emit):
        OutputGenerator.beginFeature(self, interface, emit)
        self.OldVersion = (self.featureName in ['GL_VERSION_1_0', 'GL_VERSION_1_1'])
    def endFeature(self):
        OutputGenerator.endFeature(self)
    def genType(self, typeinfo, name):
        OutputGenerator.genType(self, typeinfo, name)
    def genEnum(self, enuminfo, name):
        OutputGenerator.genEnum(self, enuminfo, name)
    def genCmd(self, cmd, name):
        OutputGenerator.genCmd(self, cmd, name)

        rettype=ParseCmdRettype(cmd)
        self.outFile.write("%s %sWrapper("%(rettype, name))
        plist=ParseCmdParams(cmd)

        Comma=""
        if len(plist):
            for ptype, pname in plist:
                self.outFile.write("%s%s"%(Comma, ptype))
                Comma=", "
        else:
            self.outFile.write("void")

        self.outFile.write(")\n{\n")

        # for GL 1.0 and 1.1 functions, generate stdcall thunk wrappers which call the function directly
        if self.OldVersion:
            if rettype.lower()=="void":
                self.outFile.write("  %s( "%(name))
            else:
                self.outFile.write("  return %s( "%(name))

            Comma=""
            for ptype, pname in plist:
                self.outFile.write("%s%s"%(Comma, pname))
                Comma=", "

        # for GL 1.2+ functions, generate wrappers which use wglGetProcAddress()
        else:
            if rettype.lower()=="void":
                self.outFile.write('  RESOLVE(PFN%sPROC, "%s");\n'%(name.upper(), name))
                self.outFile.write("  RESOLVED_PROC(PFN%sPROC)( """%(name.upper()))
            else:
                self.outFile.write('  RESOLVE_RET(PFN%sPROC, "%s", FALSE);\n'%(name.upper(), name))
                self.outFile.write("  return RESOLVED_PROC(PFN%sPROC)("%(name.upper()))

            Comma=""
            for ptype, pname in plist:
                self.outFile.write("%s%s"%(Comma, pname))
                Comma=", "
        self.outFile.write(" );\n}\n\n")

class ThunkDefsOutputGenerator(OutputGenerator):
    def __init__(self,
                 errFile = sys.stderr,
                 warnFile = sys.stderr,
                 diagFile = sys.stdout):
        OutputGenerator.__init__(self, errFile, warnFile, diagFile)
    def beginFile(self, genOpts):
        self.outFile.write("EXPORTS\n"); # this must be the first line for libtool to realize this is a .def file
        self.outFile.write('; Automatically generated from %s - DO NOT EDIT\n\n'%regFilename)
    def endFile(self):
        pass
    def beginFeature(self, interface, emit):
        OutputGenerator.beginFeature(self, interface, emit)
    def endFeature(self):
        OutputGenerator.endFeature(self)
    def genType(self, typeinfo, name):
        OutputGenerator.genType(self, typeinfo, name)
    def genEnum(self, enuminfo, name):
        OutputGenerator.genEnum(self, enuminfo, name)
    def genCmd(self, cmd, name):
        OutputGenerator.genCmd(self, cmd, name)

        # export the wrapper function with the name of the function it wraps
        self.outFile.write("%s = %sWrapper\n"%(name, name))

class ShimOutputGenerator(OutputGenerator):
    def __init__(self,
                 errFile = sys.stderr,
                 warnFile = sys.stderr,
                 diagFile = sys.stdout):
        OutputGenerator.__init__(self, errFile, warnFile, diagFile)
    def beginFile(self, genOpts):
        self.outFile.write('/* Automatically generated from %s - DO NOT EDIT */\n\n'%regFilename)
    def endFile(self):
        pass
    def beginFeature(self, interface, emit):
        OutputGenerator.beginFeature(self, interface, emit)
        self.OldVersion = (self.featureName in ['GL_VERSION_1_0', 'GL_VERSION_1_1', 'GL_VERSION_1_2', 'GL_ARB_imaging', 'GL_ARB_multitexture', 'GL_ARB_texture_compression'])
    def endFeature(self):
        OutputGenerator.endFeature(self)
    def genType(self, typeinfo, name):
        OutputGenerator.genType(self, typeinfo, name)
    def genEnum(self, enuminfo, name):
        OutputGenerator.genEnum(self, enuminfo, name)
    def genCmd(self, cmd, name):
        OutputGenerator.genCmd(self, cmd, name)

        if not self.OldVersion:
            return

        # for GL functions which are in the ABI, generate a shim which calls the function via GetProcAddress
        rettype=ParseCmdRettype(cmd)
        self.outFile.write("%s %s("%(rettype, name))
        plist=ParseCmdParams(cmd)

        Comma=""
        if len(plist):
            for ptype, pname in plist:
                self.outFile.write("%s%s"%(Comma, ptype))
                Comma=", "
        else:
            self.outFile.write("void")

        self.outFile.write(")\n{\n")

        self.outFile.write('  typedef %s (* PFN%sPROC)(' % (rettype, name.upper()))

        if len(plist):
            Comma=""
            for ptype, pname in plist:
                self.outFile.write("%s %s"%(Comma, ptype))
                Comma=", "
        else:
            self.outFile.write("void")

        self.outFile.write(');\n')

        if rettype.lower()=="void":
            self.outFile.write('  RESOLVE(PFN%sPROC, "%s");\n'%(name.upper(), name))
            self.outFile.write('  RESOLVED_PROC(')
        else:
            self.outFile.write('  RESOLVE_RET(PFN%sPROC, "%s", 0);\n'%(name.upper(), name))
            self.outFile.write('  return RESOLVED_PROC(')

        Comma=""
        for ptype, pname in plist:
            self.outFile.write("%s%s"%(Comma, pname))
            Comma=", "

        self.outFile.write(" );\n}\n\n")

def genHeaders():
    outFile = open(outFilename,"w")

    if preresolve:
        gen = PreResolveOutputGenerator(errFile=errWarn,
                                        warnFile=errWarn,
                                        diagFile=diag)
        gen.outFile=outFile
        reg.setGenerator(gen)
        reg.apiGen(genOpts)

    if wrapper:
        gen = WrapperOutputGenerator(errFile=errWarn,
                                     warnFile=errWarn,
                                     diagFile=diag)
        gen.outFile=outFile
        reg.setGenerator(gen)
        reg.apiGen(genOpts)

    if shim:
        gen = ShimOutputGenerator(errFile=errWarn,
                                  warnFile=errWarn,
                                  diagFile=diag)
        gen.outFile=outFile
        reg.setGenerator(gen)
        reg.apiGen(genOpts)

    if thunk:
        gen = ThunkOutputGenerator(errFile=errWarn,
                                   warnFile=errWarn,
                                   diagFile=diag)
        gen.outFile=outFile
        reg.setGenerator(gen)
        reg.apiGen(genOpts)


    if thunkdefs:
        gen = ThunkDefsOutputGenerator(errFile=errWarn,
                                       warnFile=errWarn,
                                       diagFile=diag)
        gen.outFile=outFile
        reg.setGenerator(gen)
        reg.apiGen(genOpts)

    outFile.close()

genHeaders()
