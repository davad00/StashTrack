export const FALLBACK_VERSION_TAG = 'v0.6'
export const FALLBACK_INSTALLER_URL =
  'https://github.com/davad00/StashTrack/releases/download/v0.6/StashTrackv0.6Setup.exe'

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
  return release.assets?.find((asset) =>
    /^StashTrackv.+Setup\.exe$/i.test(asset.name),
  )
}

export async function getLatestInstallerRelease(): Promise<LatestInstallerRelease> {
  try {
    const response = await fetch(
      'https://api.github.com/repos/davad00/StashTrack/releases/latest',
      {
        headers: {
          Accept: 'application/vnd.github+json',
          'User-Agent': 'stashtrack.n9records.com',
        },
        next: { revalidate: 300 },
      },
    )

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
        release.html_url ??
        `https://github.com/davad00/StashTrack/releases/tag/${release.tag_name}`,
      installerUrl: installer.browser_download_url,
      fallback: false,
    }
  } catch {
    return {
      versionTag: FALLBACK_VERSION_TAG,
      releasePageUrl: `https://github.com/davad00/StashTrack/releases/tag/${FALLBACK_VERSION_TAG}`,
      installerUrl: FALLBACK_INSTALLER_URL,
      fallback: true,
    }
  }
}
