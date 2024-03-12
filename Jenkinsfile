#!/usr/bin/env groovy

// This pipeline is designed to run on Esri-internal CI infrastructure.

@Library('psl')
import com.esri.zrh.jenkins.PipelineSupportLibrary 
import com.esri.zrh.jenkins.PslFactory 
import com.esri.zrh.jenkins.psl.UploadTrackingPsl
import com.esri.zrh.jenkins.JenkinsTools
import com.esri.zrh.jenkins.ToolInfo
import com.esri.zrh.jenkins.ce.CityEnginePipelineLibrary
import com.esri.zrh.jenkins.ce.PrtAppPipelineLibrary
import groovy.transform.Field

@Field def psl = PslFactory.create(this, UploadTrackingPsl.ID)
@Field def cepl = new CityEnginePipelineLibrary(this, psl)
@Field def papl = new PrtAppPipelineLibrary(cepl)


// -- PIPELINE SETUP

psl.runsHere('production')
env.PIPELINE_ARCHIVING_ALLOWED = "true"
properties([ disableConcurrentBuilds() ])


// -- CONFIGURATION

@Field final String REPO = 'git@github.com:esri/puma.git'
@Field final String SOURCE = 'puma.git'
@Field final String SOURCE_STASH = 'puma-sources'

@Field final String DOCKER_AGENT_WINDOWS = 'win19-64-d'
@Field final String DOCKER_WS_WINDOWS = "c:/temp/puma/ws"

@Field final Map WINDOWS_DOCKER_CONFIG = [ ba: DOCKER_AGENT_WINDOWS, ws: DOCKER_WS_WINDOWS ]
@Field final Map WINDOWS_NATIVE_CONFIG = [ os: cepl.CFG_OS_WIN10, bc: cepl.CFG_BC_REL, tc: cepl.CFG_TC_VC1427, cc: cepl.CFG_CC_OPT, arch: cepl.CFG_ARCH_X86_64 ]

@Field final Map RHINO6_CONFIG = [ rh: '6.35.21222.17001', rhsdk: '6.35.21222.17001', py: '3.9.13' ]
@Field final Map RHINO7_CONFIG = [ rh: '7.17.22102.5001',  rhsdk: '7.19.22165.13001', py: '3.9.13' ]
@Field final Map RHINO8_CONFIG = [ rh: '8.3.24009.15001', rhsdk: '8.3.24009.15001', py: '3.9.13' ]

@Field final List CONFIGS_PREPARE = [
    WINDOWS_NATIVE_CONFIG
]

@Field final List CONFIGS = [
	composeConfig(RHINO6_CONFIG, WINDOWS_NATIVE_CONFIG, WINDOWS_DOCKER_CONFIG),
	composeConfig(RHINO7_CONFIG, WINDOWS_NATIVE_CONFIG, WINDOWS_DOCKER_CONFIG),
	composeConfig(RHINO8_CONFIG, WINDOWS_NATIVE_CONFIG, WINDOWS_DOCKER_CONFIG),
]


// -- THE PIPELINE

stage('prepare') {
	cepl.runParallel(taskGenPrepare())
}

stage('build') {
	cepl.runParallel(taskGenPuma())
}


// -- TASK GENERATORS

Map taskGenPrepare() {
	Map tasks = [:]
	tasks << cepl.generateTasks('prepare', this.&taskPrepare, CONFIGS_PREPARE)
	return tasks
}

Map taskGenPuma() {
	return cepl.generateTasks('build', this.&taskBuildPuma, CONFIGS)
}


// -- TASK IMPLEMENTATIONS

def taskPrepare(cfg) {
 	cepl.cleanCurrentDir()
	papl.checkout(REPO, env.BRANCH_NAME)
	stash(name: SOURCE_STASH)
}

def taskBuildPuma(cfg) {
	cepl.cleanCurrentDir()
	unstash(name: SOURCE_STASH)

	final String tag = "rh${cfg.rh}-rhsdk${cfg.rhsdk}-py${cfg.py}"
	final String image = "zrh-dreg-sp-1.esri.com/puma/puma-toolchain:${tag}"

	final String buildCmd = "ci_build.cmd"
	final String containerName = "puma-build-rh${cfg.rh}-${env.BRANCH_NAME.replaceAll('/', '_')}-b${BUILD_ID}"

	String workDir = "${cfg.ws}/${SOURCE}"
	Map dirMap = [ (env.WORKSPACE) : cfg.ws ]

	final String rhinoMajorVersion = cfg.rh.tokenize('.')[0]
	Map envMap = [ 'RHINO_VER_MAJOR' : rhinoMajorVersion, 'PUMA_VER_BUILD' : BUILD_ID ]

	runDockerCmd(cfg, image, containerName, dirMap, envMap, workDir, buildCmd)

	manifest = readYaml(file: "${SOURCE}/manifest.yml") // called within publish, different cwd

	// build final package version as <major>.<minor>.<revision>.<build number>
	final List pkgVerComponents = manifest.version.tokenize('.')
	pkgVerComponents[3] = BUILD_ID
	final String pkgVer = pkgVerComponents.join('.')

	def getVersion = { return pkgVer }
	def getClassifier = { 
		List rhVerComp = cfg.rh.tokenize('.')
		return "rh${rhVerComp[0]}_${rhVerComp[1]}-win"
	}
	papl.publish('puma', env.BRANCH_NAME, '*.yak', getVersion, cfg, getClassifier, "${SOURCE}/packages")
	papl.publish('puma', env.BRANCH_NAME, '*.rhi', getVersion, cfg, getClassifier, "${SOURCE}/packages")
}


// -- HELPERS

@NonCPS
Map composeConfig(rh, tc, dc) {
	return rh + tc + dc + [ grp: "rh${rh['rh']}+rhsdk${rh['rhsdk']}" ]
}

def runDockerCmd(Map cfg, String image, String containerName, Map dirMap, Map envMap, String workDir, String cmd) {
	String dirMapStrArgs = ""
	dirMap.each { k,v -> dirMapStrArgs += " -v \"${k}:${v}\"" }

	String envMapStrArgs = ''
	envMap.each { k, v -> envMapStrArgs += " --env ${k}=${v}" }

	String runArgs = '--pull always --rm'
	runArgs += " --name ${containerName}"
	runArgs += dirMapStrArgs
	runArgs += " -w ${workDir}"
	runArgs += envMapStrArgs
	runArgs += " ${image}"
	runArgs += isUnix() ? " bash -c '${cmd}'" : " cmd /c \"${cmd}\""

	psl.runCmd("docker run ${runArgs}")
}
