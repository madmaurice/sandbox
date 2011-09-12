"""
Filename: file-sync.py

Created: 09/10/2011 11:17:37 AM

Author: Jonathan Maurice
Email: madmaurice [at] hotmail [dot] com

Description: Copy multiple file or directories to remote computer by using scp.
For this script to work, ssh on the remotes computer need to be configured to
accept automatic login from the client.

"""

from optparse import OptionParser
from urlparse import urlparse
import subprocess
import ConfigParser
import logging
import shutil
import signal
import sys
import os


class ParserException(Exception):
  """Fail to parse configuration file"""
  def __init__(self, msg):
    super(ParserException, self).__init__()
    self.msg = msg

  def __str__(self):
    return self.msg


class InvalidConfigurationException(Exception):
  """Configuration is invalid"""
  def __init__(self, msg):
    super(InvalidConfigurationException, self).__init__()
    self.msg = msg
    
  def __str__(self):
    return self.msg


class TimeoutException(Exception):
  """Subprocess timeout"""
  def __init__(self):
    super(TimeoutException, self).__init__()
    

class ObjectWithLogger(object):
  """Object that has a logger attribute whose name is the class name"""
  def __init__(self):
    super(ObjectWithLogger, self).__init__()
    self.logger = logging.getLogger(self.__class__.__name__)
    

class SshAuth(ObjectWithLogger):
  """Contain basic parameters of an ssh connection"""

  def __init__(self, userName, remoteHost, privKey=None):
    super(SshAuth, self).__init__()
    self.userName = userName
    self.remoteHost = remoteHost
    self.privKey = privKey

  def __str__(self):
    return "%s@%s" % (self.userName, self.remoteHost)

  def test(self, dryrun=False):
    """Test that the connection can be made"""
    sshCmd = SshCommand(self, 'ls', dryrun)
    return sshCmd.run()
    

class SystemCommand(ObjectWithLogger):
  """Run and manage subprocess"""
  TIMEOUT_MIN = 5

  def __init__(self, cmd, dryrun=False, timeout=TIMEOUT_MIN):
    super(SystemCommand, self).__init__()
    self.cmd = cmd
    self.dryrun = dryrun
    self.timeout = timeout


  def run(self):
    
    if self.dryrun:
      self.logger.info("Running command : %s" % self.cmd)
      return True

    proc = subprocess.Popen(self.cmd.split(),
                            stdin=subprocess.PIPE,
                            stdout=subprocess.PIPE,
                            stderr=subprocess.PIPE)

    #Start timer to avoid infinite wait
    signal.signal(signal.SIGALRM, timeoutHandler)
    signal.alarm(self.TIMEOUT_MIN * 60)

    try:
      (stdout, stderr) = proc.communicate()
      signal.alarm(0) #reset the alarm
    except TimeoutException:
      self.logger.error("Command timed out: %s" % self.cmd)
      return False

    if proc.returncode != 0:
      #Command failed
      self.logger.error("Command failed: %s" % self.cmd)
      return False

    return True
    

    
class BaseRemoteCommand(ObjectWithLogger):
  """Abstract remote command, run command set with _setCmd()"""
  def __init__(self, sshAuth, dryrun, *args, **kwargs):
    #TODO: timeout as arguments
    super(BaseRemoteCommand, self).__init__()
    self.sshAuth = sshAuth
    self.dryrun = dryrun
    self._cmd = None

  def getCmd(self):
    return self._cmd
  
  def _setCmd(self):
    """Meant to be overwriten by subclasses"""
    raise NotImplementedError('run is not implemented, abstract class')

  def run(self):

    self._setCmd()

    if self._cmd is None:
      return False

    subProc = SystemCommand(self._cmd, self.dryrun)
    return subProc.run()
    
    

class SshCommand(BaseRemoteCommand):
  """Command to be run remotely"""
  def __init__(self, sshAuth, sshCmd, dryrun=False):
    super(SshCommand, self).__init__(sshAuth, dryrun)
    self.sshCmd = sshCmd

  def _setCmd(self):
    self._cmd = "ssh "

    #PrivateKey
    if self.sshAuth.privKey is not None:
      self._cmd += "-i %s " % self.sshAuth.privKey

    #Login
    self._cmd += "%s@%s " % (self.sshAuth.userName, self.sshAuth.remoteHost)

    self._cmd += self.sshCmd



class ScpCopy(BaseRemoteCommand):
  """File transfer with remote host using scp"""
  def __init__(self, sshAuth, srcPath, dstPath, dryrun=False):
    super(ScpCopy, self).__init__(sshAuth, dryrun)
    self.srcPath = srcPath
    self.dstPath = dstPath

  def __str__(self):
    if os.path.isdir(self.srcPath):
      qual = 'Directory'
    else:
      qual = 'File'
    return "%s %s to %s:%s" % (qual, self.srcPath, self.sshAuth, self.dstPath)

  def _setCmd(self):
    self._cmd = "scp "

    #If src is a directory add -r
    if os.path.isdir(self.srcPath):
      self._cmd += "-r "

    #PrivateKey
    if self.sshAuth.privKey is not None:
      self._cmd += "-i %s " % self.sshAuth.privKey

    self._cmd += "%s " % self.srcPath

    #Dst
    self._cmd += "%s@%s:" % (self.sshAuth.userName, self.sshAuth.remoteHost)
    self._cmd += "%s" % self.dstPath



class SvnCheckout(ObjectWithLogger):
  """Checkout a subversion repo"""
  def __init__(self, workDir, svnUrl, coPath, dryrun=False):
    super(SvnCheckout, self).__init__()
    self.workDir = workDir
    self.svnUrl = svnUrl
    self.coPath = coPath
    self.dryrun = dryrun
    self._cmd = "svn co %s %s" % (self.svnUrl, self.coPath)

  def run(self):
    #TODO exception handling
    retVal = True

    cwd = os.getcwd()
    os.chdir(self.workDir)

    if os.path.exists(self.coPath) and not self.dryrun:
      self.logger.info("Deleting %s for svn checkout of %s", 
                       os.path.join(self.workDir, self.coPath),
                       self.svnUrl)
      shutil.rmtree(self.coPath)

    subProc = SystemCommand(self._cmd, self.dryrun)
    self.logger.info("Doing svn checkout of %s in %s/%s", self.svnUrl,
                     self.workDir, self.coPath)

    if not subProc.run():
      self.logger.error("Failed to checkout %s", self.svnUrl)
      retVal = False

    os.chdir(cwd)

    return retVal
    

    
class FTSection(object):
  """Represent a section of the configuration file"""

  OPT_SRC_PATH = 'srcPath'
  OPT_REMOTE_HOST = 'remoteHost'
  OPT_REMOTE_OUTDIR = 'remoteOutDir'
  OPT_REMOTE_ERASE = 'eraseRemoteDst'

  def __init__(self, name, srcPath, remoteHost, remoteOutDir, eraseRemoteDir):
    super(FTSection, self).__init__()
    self.name = name
    self.srcPath = srcPath
    self.remoteHostList= remoteHost.split()
    self.remoteOutDir = remoteOutDir
    self.eraseRemoteDir = eraseRemoteDir
    


class FTConfig(ObjectWithLogger):
  """Represent the whole file sync configuration"""

  CFG_SECTION = 'config'
  OPT_PRIVKEY = 'privKey'
  OPT_USER_LOGIN = 'userLogin'

  def __init__(self, cfgFilePath):
    super(FTConfig, self).__init__()
    self.cfgFilePath = cfgFilePath
    self.privKey = None
    self.userLogin = None
    self._sectionList = list()
    self._parser = ConfigParser.SafeConfigParser()

    
  def parseConfigFile(self):
    """Read configuration and set data member. Each sections (except the config)
       will create a new FTSection and append it to the _sectionList."""
    
    if not os.path.isfile(self.cfgFilePath):
      raise ParserException("Configuration file %s does not exist" % 
                            self.cfgFilePath)

    if not self._parser.read(self.cfgFilePath):
      raise ParserException("Failed to parse configuration file %s, " \
                            "syntax seems invalid" % self.cfgFilePath)

    #Make sure that the config file is valid
    self._validateConfigFile()

    #Parse the config section first (if any)
    if self._parser.has_option(self.CFG_SECTION, self.OPT_PRIVKEY):
      self.privKey = self._parser.get(self.CFG_SECTION, self.OPT_PRIVKEY)

    if self._parser.has_option(self.CFG_SECTION, self.OPT_USER_LOGIN):
      self.userLogin = self._parser.get(self.CFG_SECTION, self.OPT_USER_LOGIN)

    for section in self._parser.sections():
      #Skip config section
      if section == self.CFG_SECTION:
        continue

      self._sectionList.append(\
        FTSection(section, 
                  self._parser.get(section, FTSection.OPT_SRC_PATH),
                  self._parser.get(section, FTSection.OPT_REMOTE_HOST),
                  self._parser.get(section, FTSection.OPT_REMOTE_OUTDIR),
                  self._parser.getboolean(section, FTSection.OPT_REMOTE_ERASE)))


  @property
  def ftSections(self):
    """Return a list of the file transfer sections"""
    return self._sectionList


  def _validateConfigFile(self):
    """Validate the configuration file syntax"""
    sections = self._parser.sections()

    if sections.count(self.CFG_SECTION) > 1:
      raise InvalidConfigurationException("Configuration file %s is invalid, "\
                                          "only one %s section is allowed." %
                                          (self.cfgFilePath, self.CFG_SECTION))

    #Remove config section
    if self.CFG_SECTION in sections:
      sections.remove(self.CFG_SECTION)

    #Following section fields are mandatory
    for section in sections:
      if not self._parser.has_option(section, FTSection.OPT_SRC_PATH):
        raise InvalidConfigurationException("Configuration file %s is invalid, "\
                                            "section %s is missing field %s." %
                                            (self.cfgFilePath, section,
                                             FTSection.OPT_SRC_PATH))

      if not self._parser.has_option(section, FTSection.OPT_REMOTE_HOST):
        raise InvalidConfigurationException("Configuration file %s is invalid, "\
                                            "section %s is missing field %s." %
                                            (self.cfgFilePath, section,
                                             FTSection.OPT_REMOTE_HOST))

      if not self._parser.has_option(section, FTSection.OPT_REMOTE_OUTDIR):
        raise InvalidConfigurationException("Configuration file %s is invalid, "\
                                            "section %s is missing field %s." %
                                            (self.cfgFilePath, section,
                                             FTSection.OPT_REMOTE_OUTDIR))

      if not self._parser.has_option(section, FTSection.OPT_REMOTE_ERASE):
        raise InvalidConfigurationException("Configuration file %s is invalid, "\
                                            "section %s is missing field %s." %
                                            (self.cfgFilePath, section,
                                             FTSection.OPT_REMOTE_ERASE))

      #Make sure that erase option is a boolean
      try:
        self._parser.getboolean(section, FTSection.OPT_REMOTE_ERASE)
      except ValueError:
        raise InvalidConfigurationException("Configuration file %s is invalid, "\
                                            "field %s must be boolean." %
                                            (self.cfgFilePath, 
                                            FTSection.OPT_REMOTE_ERASE))



class FileSync(ObjectWithLogger):
  """Sync files base on configuration file"""
  #TODO: better error checking/reporting mainly in run()
  #More logging

  SVN_DIR = 'fsync-dir'

  def __init__(self, configFile, dryrun, testAuth, logFile):
    super(FileSync, self).__init__()
    self.configFile = configFile
    self.dryrun = dryrun
    self.testAuth = testAuth
    self.logFile = logFile
    self.thisDir = os.path.dirname(os.path.abspath(__file__))
    self.svnDir = os.path.join(self.thisDir, self.SVN_DIR)
    self._userLogin = None
    self._ftConfig = None

    
  def init(self):
    """Initialize all components required to operate"""
    if not self._setDirectories():
      return False

    if not self._setUpLogging():
      return False

    if not os.path.exists(self.configFile):
      self.logger.critical("Configuration file %s does not exist",
                           self.configFile)
      return False


    self._ftConfig = FTConfig(self.configFile)
    self._ftConfig.parseConfigFile()

    if self._ftConfig.userLogin is not None:
      self._userLogin = self._ftConfig.userLogin
    else:
      self._userLogin = os.environ['USER']

    return True


  def __del__(self):
    """Destructor, make sure that temporary checkout directory is deleted"""
    if os.path.exists(self.svnDir) and not self.dryrun:
      shutil.rmtree(self.svnDir)


  def run(self):
    """Execute file syncing"""

    #Check if this is a authentication test
    if self.testAuth:
      return self._testAuth()

    for section in self._ftConfig.ftSections:

      cpSrc = None
      cpDst = section.remoteOutDir

      #Check if section.srcPath is a svn url or a local path
      if self._isSvnUrl(section.srcPath):
        svnCo = SvnCheckout(self.svnDir, section.srcPath, 
                            section.name, self.dryrun) 
        if not svnCo.run():
          continue

        cpSrc = os.path.join(self.svnDir, section.name)

      else:
        cpSrc = section.srcPath


      for remoteHost in section.remoteHostList:

        #Create sshAuth for this remoteHost
        sshAuth = SshAuth(self._userLogin, remoteHost, 
                          self._ftConfig.privKey)

        #Check if remote directory must be erased
        if section.eraseRemoteDir:
          self._eraseRemoteDir(sshAuth, section)

        fCopy = ScpCopy(sshAuth, cpSrc, cpDst, self.dryrun)
        self.logger.info("Copying : %s", fCopy)
        if not fCopy.run():
          self.logger.error("Failed to copy : %s", fCopy)

    return True


  def _isSvnUrl(self, path):
    """Return true if path argument is a svn or http url"""
    parseResult = urlparse(path)

    if parseResult.scheme == 'svn' or parseResult.scheme == 'http':
      return True
    else:
      return False


  def _eraseRemoteDir(self, sshAuth, ftSection):
    """Delete a directory on remote host.
       Remote directory is composed of the last part of the srcPath appended
       to remoteOutDir"""

    if self._isSvnUrl(ftSection.srcPath):
      remoteDir = os.path.join(ftSection.remoteOutDir, ftSection.name)
    else:
      #Remove trailing / if any
      srcDir = ftSection.srcPath.rstrip('/')
      #Retrieve last part of srcPath
      srcDirName = os.path.basename(srcDir)

      remoteDir = os.path.join(ftSection.remoteOutDir, srcDirName)

    self.logger.info("Erasing directory %s on remote host %s", remoteDir,
                     sshAuth)

    cmd = "rm -rf %s" % remoteDir
    sshCmd = SshCommand(sshAuth, cmd, self.dryrun)

    return sshCmd.run()


  def _setDirectories(self):
    """Set current directory to the same as this file and create working dir"""
    try:
      os.chdir(self.thisDir)
    except OSError:
      self.logger.critical("Failed to change current directory to %s",
                           self.thisDir)
      return False

    if not os.path.exists(self.svnDir):
      try:
        os.mkdir(self.svnDir)
      except OSError:
        self.logger.critical("Failed to create directory %s", self.svnDir)
        return False


    return True


  def _setUpLogging(self):
    """Configure logging"""
    #Set log level to INFO
    rootLogger = logging.getLogger()
    rootLogger.setLevel(logging.INFO)

    formatter = logging.Formatter('%(asctime)s - %(name)s - %(levelname)s - '\
                                  '%(message)s')

    #Stdout handler
    stdoutHandler = logging.StreamHandler()
    stdoutHandler.setFormatter(formatter)
    rootLogger.addHandler(stdoutHandler)

    if self.logFile is not None:
      #Log to file, overwrite previous one if any...
      fileHandler = logging.FileHandler(self.logFile, mode='w')
      fileHandler.setFormatter(formatter)
      rootLogger.addHandler(fileHandler)

    return True


  def _testAuth(self):
    remoteList = list()

    #Create a list of all the remote host
    for section in self._ftConfig.ftSections:

      for remoteHost in section.remoteHostList:
        if remoteHost in remoteList:
          continue
        remoteList.append(remoteHost)

    for remote in remoteList:
      sshAuth = SshAuth(self._userLogin, remote, self._ftConfig.privKey)

      if sshAuth.test(self.dryrun):
        self.logger.info("Success : Authentication of %s", sshAuth)
      else:
        self.logger.warn("Fail : Authentication of %s  "\
                          "perhaps private key '%s' is not authorized!", 
                          sshAuth, self._ftConfig.privKey)

    
def timeoutHandler(signum, frame):
  """Called on subprocess timeout"""
  raise TimeoutException()


def main():
  usage = "usage: %prog [options]"
  optParser = OptionParser(usage)
  optParser.add_option('-c', '--config-file', dest='configFile',
                       action="store", default='file-sync.cfg',
                       metavar="FILE",
                       help="Path to configuration file, if not provided "\
                            "configuration file is '%default'")
  optParser.add_option('-d', '--dry-run', dest='dryrun',
                       action="store_true", default=False,
                       help="Print what would be done, without doing it")
  optParser.add_option('-t', '--test-auth', dest='testAuth',
                       action="store_true", default=False,
                       help="Test ssh authentication in configuration file")
  optParser.add_option('-l', '--log-file', dest='logFile',
                       action="store", metavar="FILE",
                       help="Path to log file, if not provided will print to "\
                            "stdout only")

  (options, args) = optParser.parse_args()

  
  fsync = FileSync(options.configFile,
                   options.dryrun,
                   options.testAuth,
                   options.logFile)

  if not fsync.init():
    logging.critical("Failed to initialize, aborting!")
    sys.exit(1)

  if not fsync.run():
    sys.exit(1)


if __name__ == '__main__':
  main()
