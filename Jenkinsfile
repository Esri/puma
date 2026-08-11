#!/usr/bin/env groovy

// This pipeline is designed to run on Esri-internal CI infrastructure.

@Library('psl')
import com.esri.zrh.jenkins.PipelineSupportLibrary as PSL
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

@Field final String SOURCE = 'cityengine_for_rhino.git'
@Field final String REPO = "https://github.com/esri/${SOURCE}"
@Field final String SOURCE_STASH = 'puma-sources'

@Field final String DOCKER_AGENT_WINDOWS = 'win19-64-d'
@Field final String DOCKER_WS_WINDOWS = "c:/temp/puma/ws"

@Field final Map WINDOWS_DOCKER_CONFIG = [ ba: DOCKER_AGENT_WINDOWS, ws: DOCKER_WS_WINDOWS ]
@Field final Map WINDOWS_NATIVE_CONFIG = [ os: cepl.CFG_OS_WIN10, bc: cepl.CFG_BC_REL, tc: cepl.CFG_TC_VC1444, cc: cepl.CFG_CC_OPT, arch: cepl.CFG_ARCH_X86_64 ]

@Field final Map RHINO7_CONFIG = [ rh: '7.17.22102.5001',  rhsdk: '7.19.22165.13001', py: '3.9.13' ]
@Field final Map RHINO8_CONFIG = [ rh: '8.8.24163.12481',  rhsdk: '8.8.24163.12481',  py: '3.9.13' ]
@Field final int DOCKER_IMAGE_REVISION = 2 // see counter part in the containers repository

@Field final List CONFIGS_PREPARE = [
    WINDOWS_NATIVE_CONFIG
]

@Field final List CONFIGS = [
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
	updateVersionProperties("${SOURCE}/version.properties", BUILD_ID)
	stash(name: SOURCE_STASH)
}

def taskBuildPuma(cfg) {
	cepl.cleanCurrentDir()
	unstash(name: SOURCE_STASH)

	final String tag = "rh${cfg.rh}-rhsdk${cfg.rhsdk}-py${cfg.py}-v${DOCKER_IMAGE_REVISION}"
	final String image = "${PSL.HARBOR_NAME}/puma/puma-toolchain:${tag}"

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
	papl.publish('cityengine_for_rhino', env.BRANCH_NAME, '*.yak', getVersion, cfg, getClassifier, "${SOURCE}/packages")
	papl.publish('cityengine_for_rhino', env.BRANCH_NAME, '*.rhi', getVersion, cfg, getClassifier, "${SOURCE}/packages")
}


// -- HELPERS

@NonCPS
Map composeConfig(rh, tc, dc) {
	return rh + tc + dc + [ grp: "rh${rh['rh']}+rhsdk${rh['rhsdk']}" ]
}

def updateVersionProperties(String propFile, String buildId) {
	def versionProps = readProperties(file: propFile)
	versionProps.VERSION_BUILD = buildId
	def propsText = versionProps.collect{ entry->entry.key+"="+entry.value }.join('\n')
	writeFile(file: propFile, text: propsText)
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
