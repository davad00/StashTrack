const GITHUB_REPO = 'davad00/StashTrack'
const LATEST_RELEASE_API_URL = `https://api.github.com/repos/${GITHUB_REPO}/releases/latest`
const LATEST_RELEASE_PAGE_URL = `https://github.com/${GITHUB_REPO}/releases/latest`
const STABLE_INSTALLER_ASSET_NAME = 'StashTrackSetup.exe'

export const FALLBACK_VERSION_TAG = 'latest'
export const FALLBACK_INSTALLER_URL =
  `https://github.com/${GITHUB_REPO}/releases/latest/download/${STABLE_INSTALLER_ASSET_NAME}`

type GitHubReleaseAsset = {
  name: string
  browser_download_url: string
}

type GitHubRelease = {
  tag_name?: string
  html_url?: string
  assets?: GitHubReleaseAsset[]
}

export type LatestInstallerRelease = {
  versionTag: string
  releasePageUrl: string
  installerUrl: string
  fallback: boolean
}

function findWindowsInstallerAsset(release: GitHubRelease) {
  return (
    release.assets?.find(
      (asset) => asset.name.toLowerCase() === STABLE_INSTALLER_ASSET_NAME.toLowerCase(),
    ) ??
    release.assets?.find((asset) => /^StashTrackv.+Setup\.exe$/i.test(asset.name))
  )
}

function releasePageUrlForTag(versionTag: string) {
  return `https://github.com/${GITHUB_REPO}/releases/tag/${versionTag}`
}

function versionedInstallerUrlForTag(versionTag: string) {
  return `https://github.com/${GITHUB_REPO}/releases/download/${versionTag}/StashTrack${versionTag}Setup.exe`
}

function tagFromLatestRedirect(location: string | null) {
  const match = location?.match(/\/releases\/tag\/([^/?#]+)/)
  return match?.[1] ? decodeURIComponent(match[1]) : undefined
}

async function getLatestTagFromGitHubRedirect() {
  const response = await fetch(LATEST_RELEASE_PAGE_URL, {
    method: 'GET',
    redirect: 'manual',
    cache: 'no-store',
    headers: {
      'User-Agent': 'stashtrack.n9records.com',
    },
  })

  return tagFromLatestRedirect(response.headers.get('location')) ?? tagFromLatestRedirect(response.url)
}

export async function getLatestInstallerRelease(): Promise<LatestInstallerRelease> {
  try {
    const response = await fetch(LATEST_RELEASE_API_URL, {
      headers: {
        Accept: 'application/vnd.github+json',
        'User-Agent': 'stashtrack.n9records.com',
      },
      cache: 'no-store',
    })

    if (!response.ok) {
      throw new Error(`GitHub latest release returned ${response.status}`)
    }

    const release = (await response.json()) as GitHubRelease
    const installer = findWindowsInstallerAsset(release)

    if (!release.tag_name || !installer?.browser_download_url) {
      throw new Error('Latest release is missing a StashTrack setup asset')
    }

    return {
      versionTag: release.tag_name,
      releasePageUrl:
        release.html_url ?? releasePageUrlForTag(release.tag_name),
      installerUrl: installer.browser_download_url,
      fallback: false,
    }
  } catch {
    const latestTag = await getLatestTagFromGitHubRedirect().catch(() => undefined)

    if (latestTag) {
      return {
        versionTag: latestTag,
        releasePageUrl: releasePageUrlForTag(latestTag),
        installerUrl: versionedInstallerUrlForTag(latestTag),
        fallback: false,
      }
    }

    return {
      versionTag: FALLBACK_VERSION_TAG,
      releasePageUrl: LATEST_RELEASE_PAGE_URL,
      installerUrl: FALLBACK_INSTALLER_URL,
      fallback: true,
    }
  }
}
