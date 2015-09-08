import xml.dom.minidom
from iocbuilder import Substitution, AutoSubstitution, SetSimulation, Device, records, Architecture, IocDataStream
from iocbuilder.arginfo import *
from iocbuilder.modules.asyn import Asyn, AsynPort, AsynIP
from iocbuilder.modules.areaDetector import AreaDetector, _NDPluginProducerBase
from iocbuilder.modules.mca import Mca
from iocbuilder.modules.seq import Seq

__all__ = ['Dxp']

class Dxp(Device):
    Dependencies = (AreaDetector, Seq)
    LibFileList = ['dxp', 'handel']
    if Architecture() == "win32-x86":
        LibFileList += ['DLPORTIO', 'PlxApi']
        SysLibFileList = ['setupapi']
        
    DbdFileList = ['dxpSupport']
    MakefileStringList = ['LIB_INSTALLS_WIN32 += $(wildcard $(DXP)/dxpApp/firmware/xMAP_Reset/*.fdd)',
                          '# install all the firmware files from the dxp module into the lib dir']
    AutoInstantiate = True

class DxpXmap(AsynPort):
    Dependencies = (Dxp,)

    def __init__(self, name, P, BUFFERS=250, MEMORY=-1, NDET=4, NSCA=32, CHANS=16384, INIFILE="C:\\XMAP\\ini\\epics.ini"):
        self.__dict__.update(locals())
        self.__super.__init__(name)
        
    # __init__ arguments
    ArgInfo = makeArgInfo(__init__,
        name    = Simple('Object and asyn port name'),
        P       = Simple('PV prefix', str),
        NDET    = Simple('Number of detectors units in system', int),
        NSCA    = Choice('Number of SCAs', [16, 32]),
        CHANS   = Simple('Number of channels or bins in each spectrum', int),
        BUFFERS = Simple('Maximum number of NDArray buffers to be created', int),
        MEMORY  = Simple('Max memory to allocate', int),
        INIFILE = Simple('Xia ini configuration file path', str))
    
    
    def Initialise(self):
        print '# Set logging level (1=ERROR, 2=WARNING, 3=INFO, 4=DEBUG)'
        print 'xiaSetLogLevel(2)'
        print '# Point to Xia ini configuration file'
        print 'xiaInit("%(INIFILE)s")' % self.__dict__
        print 'xiaStartSystem()'
        print '# NDDXPConfig(serverName, ndetectors, maxBuffers, maxMemory)'
        print 'NDDxpConfig("%(name)s", %(NDET)d, %(BUFFERS)d, %(MEMORY)d)' % self.__dict__
        
    def PostIocInitialise(self):
        print '# Start sequencer to operate XMAP'
        print 'seq dxpMED, "P=%(P)s:, DXP=DXP, MCA=MCA, N_DETECTORS=%(NDET)d, N_SCAS=%(NSCA)d"' % self.__dict__
        print '# Sleep for 10 seconds to let initialization complete and then turn on AutoApply and do Apply manually once'
        print 'epicsThreadSleep(10.)'
        print '# Turn on AutoApply'
        print 'dbpf("%(P)s:AutoApply", "Yes")' % self.__dict__
        print '# Manually do Apply once'
        print 'dbpf("%(P)s:Apply", "1")' % self.__dict__
        print '# Seems to be necessary to resend AutoPixelsPerBuffer to read back correctly from Handel'
        print 'dbpf("%(P)s:AutoPixelsPerBuffer.PROC", "1")' % self.__dict__


class XmapBufferTemplate:
    """Dummy class to satisfy the template requirements..."""
    TemplateFile = "XmapBuffer"
    ArgInfo = makeArgInfo()
    args = []
 
class XmapBufferPlugin(_NDPluginProducerBase):
    """This plugin parses XMAP mapping buffers and output a series of NDArrays of [spectra,channel]
    It also attach all the XMAP scalar data parameters as NDAttributes to the output arrays"""
    Dependencies = (Mca, Dxp)
    _SpecificTemplate = XmapBufferTemplate
    def __init__(self, **args):
        self.__super.__init__(**args)

    def Initialise(self):
        print '# %(Configure)s(portName, queueSize, '\
            'NDArrayPort, NDArrayAddr, maxBuffers, ' % self.__dict__
        print '%(Configure)s("%(PORT)s", %(QUEUE)d, ' \
            '"%(NDARRAY_PORT)s", %(NDARRAY_ADDR)s, %(BUFFERS)d, ' % self.__dict__

class dxpDLS(AutoSubstitution):
    # Substitution attributes
    TemplateFile = 'dxpDLS.template'


class dxpDXP2X(AutoSubstitution):
    # Substitution attributes
    TemplateFile = 'dxpDXP2X.template'


class dxpHighLevel(AutoSubstitution):
    # Substitution attributes
    TemplateFile = 'dxpHighLevel.template'


class dxpLowLevel(AutoSubstitution):
    # Substitution attributes
    TemplateFile = 'dxpLowLevel.template'


class dxpMapping(AutoSubstitution):
    # Substitution attributes
    TemplateFile = 'dxpMapping.template'


class dxpMED(AutoSubstitution):
    # Substitution attributes
    TemplateFile = 'dxpMED.template'


class dxpSaturn(AutoSubstitution):
    # Substitution attributes
    TemplateFile = 'dxpSaturn.template'


class dxpSCA_16(AutoSubstitution):
    # Substitution attributes
    TemplateFile = 'dxpSCA_16.template'


class dxpSCA_32(AutoSubstitution):
    # Substitution attributes
    TemplateFile = 'dxpSCA_32.template'


class dxpSystem(AutoSubstitution):
    # Substitution attributes
    TemplateFile = 'dxpSystem.template'


class mcaCallback(AutoSubstitution):
    # Substitution attributes
    TemplateFile = 'mcaCallback.template'

class dxpChannelDLS(AutoSubstitution):
    # Substitution attributes
    TemplateFile = 'dxp_channel_DLS.template'

