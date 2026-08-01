use serde::{Deserialize, Serialize};
use std::fs;
use std::path::PathBuf;
use std::process::Command;
use tauri::AppHandle;

const UPDATE_CHECK_URL: &str = "https://api.github.com/repos/noetheadynamics/openmind/releases/latest";

#[derive(Serialize, Deserialize, Clone)]
pub struct UpdateInfo {
    pub has_update: bool,
    pub current_version: String,
    pub latest_version: Option<String>,
    pub download_url: Option<String>,
    pub release_url: Option<String>,
    pub notes: Option<String>,
    pub error: Option<String>,
}

#[derive(Deserialize)]
struct GithubRelease {
    tag_name: String,
    html_url: String,
    body: Option<String>,
    assets: Vec<GithubAsset>,
}

#[derive(Deserialize)]
struct GithubAsset {
    name: String,
    browser_download_url: String,
}

fn normalize_version(v: &str) -> String {
    v.trim()
        .trim_start_matches('v')
        .trim_start_matches('V')
        .to_string()
}

fn version_greater(a: &str, b: &str) -> bool {
    let parse = |s: &str| -> Vec<u64> {
        s.split('.')
            .filter_map(|p| p.parse::<u64>().ok())
            .collect()
    };
    let va = parse(a);
    let vb = parse(b);
    for i in 0..va.len().max(vb.len()) {
        let x = *va.get(i).unwrap_or(&0);
        let y = *vb.get(i).unwrap_or(&0);
        if x != y {
            return x > y;
        }
    }
    false
}

#[tauri::command]
pub fn check_for_updates(app: AppHandle) -> UpdateInfo {
    let current = app.package_info().version.to_string();

    let client = match reqwest::blocking::Client::builder()
        .user_agent("OpenMind-Desktop-Updater/1.0")
        .build()
    {
        Ok(c) => c,
        Err(e) => {
            return UpdateInfo {
                has_update: false,
                current_version: current,
                latest_version: None,
                download_url: None,
                release_url: None,
                notes: None,
                error: Some(format!("HTTP client init failed: {e}")),
            }
        }
    };

    let resp = match client.get(UPDATE_CHECK_URL).send() {
        Ok(r) => r,
        Err(e) => {
            return UpdateInfo {
                has_update: false,
                current_version: current,
                latest_version: None,
                download_url: None,
                release_url: None,
                notes: None,
                error: Some(format!("GitHub request failed: {e}")),
            }
        }
    };

    if !resp.status().is_success() {
        let status = resp.status();
        return UpdateInfo {
            has_update: false,
            current_version: current,
            latest_version: None,
            download_url: None,
            release_url: None,
            notes: None,
            error: Some(format!("GitHub returned HTTP {status}")),
        };
    }

    let release: GithubRelease = match resp.json() {
        Ok(r) => r,
        Err(e) => {
            return UpdateInfo {
                has_update: false,
                current_version: current,
                latest_version: None,
                download_url: None,
                release_url: None,
                notes: None,
                error: Some(format!("Release parse failed: {e}")),
            }
        }
    };

    let latest = normalize_version(&release.tag_name);
    let has_update = version_greater(&latest, &current);

    let download_url = release
        .assets
        .iter()
        .find(|a| a.name.to_lowercase().ends_with("-setup.exe"))
        .map(|a| a.browser_download_url.clone())
        .or_else(|| {
            release
                .assets
                .iter()
                .find(|a| a.name.to_lowercase().ends_with(".msi"))
                .map(|a| a.browser_download_url.clone())
        });

    UpdateInfo {
        has_update,
        current_version: current,
        latest_version: Some(latest),
        download_url,
        release_url: Some(release.html_url),
        notes: release.body,
        error: None,
    }
}

fn install_dir() -> Option<PathBuf> {
    let dirs = [
        dirs_localappdata(),
        std::env::var_os("PROGRAMFILES").map(PathBuf::from),
    ];
    for d in dirs.into_iter().flatten() {
        let candidate = d.join("OpenMind");
        if candidate.join("openmind.exe").exists() {
            return Some(candidate);
        }
    }
    dirs_localappdata().map(|d| d.join("OpenMind"))
}

fn dirs_localappdata() -> Option<PathBuf> {
    std::env::var_os("LOCALAPPDATA").map(PathBuf::from)
}

#[tauri::command]
pub async fn download_and_install(
    url: String,
    app: AppHandle,
) -> Result<String, String> {
    let client = reqwest::Client::builder()
        .user_agent("OpenMind-Desktop-Updater/1.0")
        .build()
        .map_err(|e| e.to_string())?;

    let resp = client
        .get(&url)
        .send()
        .await
        .map_err(|e| format!("Download failed: {e}"))?;

    if !resp.status().is_success() {
        return Err(format!("Download returned HTTP {}", resp.status()));
    }

    let bytes = resp
        .bytes()
        .await
        .map_err(|e| format!("Read body failed: {e}"))?;

    let temp_dir = std::env::temp_dir();
    let setup_path = temp_dir.join("OpenMind_Update_setup.exe");
    fs::write(&setup_path, &bytes).map_err(|e| format!("Write failed: {e}"))?;

    let Some(install) = install_dir() else {
        return Err("Could not determine install directory".into());
    };

    let relauncher = temp_dir.join("OpenMind_Update_Apply.bat");
    let bat = format!(
        "@echo off\r\n\
         timeout /t 2 /nobreak >nul\r\n\
         start \"\" /wait \"{setup}\" /S\r\n\
         timeout /t 1 /nobreak >nul\r\n\
         start \"\" \"{exe}\"\r\n\
         del /f /q \"{setup}\" >nul 2>nul\r\n\
         del /f /q \"%~f0\" >nul 2>nul\r\n",
        setup = setup_path.display(),
        exe = install.join("openmind.exe").display()
    );
    fs::write(&relauncher, bat).map_err(|e| e.to_string())?;

    Command::new("cmd")
        .arg("/C")
        .arg(&relauncher)
        .spawn()
        .map_err(|e| format!("Relaunch script failed: {e}"))?;

    app.exit(0);
    Ok("installing".into())
}
