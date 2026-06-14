set windows-shell := ["powershell.exe", "-NoLogo", "-Command"]

workspace := justfile_directory()
lockfile := join(workspace, ".conan/latest.lock")
recipes_dir := join(workspace, ".conan/recipes")
just := just_executable() + " -f " + justfile()

help:
  @just --list

@_rec-exec command recipe *others:
  -{{ \
     if command == "remove" { \
       "conan remove -c " + recipe \
     } else { \
       "conan cache clean -b -d -t -bs " + recipe \
     } \
   }}
  {{ if others != "" { just + " " + command + " " + others } else { "" } }}

_execute command *recipes:
  @{{ \
    if recipes == "" { \
      just + " _rec-exec " + command + \
        " pichi boost boringssl botan brotli libmaxminddb openssl rapidjson" \
        } else { \
      just + " _rec-exec " + command + " " + recipes \
    } \
  }}

clean *recipes: (_execute "clean" recipes)

remove *recipes: (_execute "remove" recipes)

[unix]
_export-recipe recipe:
  #!/bin/sh
  set -eu

  name="$(echo '{{recipe}}' | sed -nE 's#^([^/]+)(/.+)*$#\1#p')"
  test -n "${name}"

  path="{{recipes_dir}}/${name}"
  test -d "${path}"

  if [ "${name}" = "{{recipe}}" ]; then
    full="$(cat '{{lockfile}}' | jq -r '.requires[] | select(test("^{{recipe}}/"))')"
  else
    full="{{recipe}}"
  fi
  test -n "${full}"

  ver="$(echo ${full} | sed -nE 's#^.+/(.+)$#\1#p')"
  test -n "${ver}"

  conan cache path "${full}" >/dev/null 2>&1 || conan export --version "${ver}" "${path}"

[windows]
_export-recipe recipe:
  #!powershell.exe

  $name = ("{{recipe}}" -split '/')[0]
  if ([string]::IsNullOrEmpty($name)) {
    exit 1
  }

  $path = "{{recipes_dir}}\$name"
  if (-not (Test-Path -Path $path -PathType Container)) {
    exit 1
  }

  if ("$name" -eq "{{recipe}}") {
    $json = Get-Content -Path "{{lockfile}}" -Raw | ConvertFrom-Json
    $full = $json.requires | Where-Object { $_ -match "^$([regex]::Escape($name))/" }
  } else {
    $full = "{{recipe}}"
  }

  $ver = ("$full" -split '/')[1]
  if ([string]::IsNullOrEmpty($ver)) {
    exit 1
  }

  conan cache path "$full" *>$null
  if (-not $?) {
    conan export --version "$ver" "{{recipes_dir}}\$name"
  }

export: \
  (_export-recipe "boringssl") \
  (_export-recipe "botan") \
  (_export-recipe "libmaxminddb")

detect-host:
  @conan profile detect -e

[unix]
detect-android ndk_ver:
  #!/bin/sh
  set -o errexit

  recipe="android-ndk/{{ndk_ver}}"
  pkg=`conan download -r conancenter -p os="{{ capitalize(os()) }}" -f json "${recipe}" | \
    jq -r "first(.\"Local Cache\".\"${recipe}\".revisions[]) | .packages | keys[0]" 2>/dev/null | \
    grep -v '^null$'`
  ndk_root="$(conan cache path ${recipe}:${pkg})/bin"

  clang=`find "${ndk_root}/toolchains" -name clang -type l`
  [ -x "$clang" ]
  clang_ver=`"${clang}" --version | sed -n 's/^.* clang version \([0-9][0-9]*\)\..*$/\1/p'`

  sysroot=`find "${ndk_root}/toolchains" -name sysroot -type d`
  [ -d "$sysroot" ]
  api_level=`find ${sysroot}/usr/lib -type d -regex ".*/[0-9]+$" | grep -oE '[0-9]+$' | sort -run | head -1`

  cat >"${HOME}/.conan2/profiles/android" <<EOF
  [settings]
  os=Android
  os.api_level=${api_level}
  compiler=clang
  compiler.version=${clang_ver}
  compiler.libcxx=c++_static
  [tool_requires]
  ${recipe}
  [conf]
  tools.android:ndk_path=${ndk_root}
  EOF

[windows]
detect-android ndk_ver:
  #!powershell.exe

  $recipe = "android-ndk/{{ndk_ver}}"
  $json = conan download -r conancenter -p os="{{ capitalize(os()) }}" -f json $recipe 2>$null | ConvertFrom-Json
  $json = $json."Local Cache".$recipe.revisions | Select-Object -First 1
  $rev = ($json.PSObject.Properties | Select-Object -First 1).Name
  $pkg = ($json.$rev.packages.PSObject.Properties | Select-Object -First 1).Name
  $ndk_root = Join-Path $(conan cache path "${recipe}:${pkg}") "bin"
  $toolchain = Join-Path "$ndk_root" "toolchains/llvm/prebuilt/windows-x86_64"

  $clang_ver = &$(Join-Path "$toolchain" "bin/clang.exe") --version | Select-String -Pattern 'clang version (\d+)\.\d+\.\d+' | ForEach-Object { $_.Matches.Groups[1].Value }
  $api_level = Get-ChildItem -Path $(Join-Path "$toolchain" "sysroot/usr/lib") -Recurse -Directory | Where-Object { $_.Name -match '^[0-9]+$' } | Select-Object -ExpandProperty Name | Sort-Object -Descending -Unique | Select-Object -First 1

  @"
  [settings]
  os=Android
  os.api_level=$api_level
  compiler=clang
  compiler.version=$clang_ver
  compiler.libcxx=c++_static
  [tool_requires]
  $recipe
  [conf]
  tools.android:ndk_path=$ndk_root
  "@ | Out-File -FilePath ~/.conan2/profiles/android

_conan cmd *args:
  @conan "{{cmd}}" -b missing -l "{{lockfile}}" --lockfile-partial \
    -o "boost/*:asio_no_deprecated=True" \
    -o "boost/*:bzip2=False" \
    -o "boost/*:system_no_deprecated=True" \
    -o "boost/*:without_atomic=True" \
    -o "boost/*:without_charconv=True" \
    -o "boost/*:without_chrono=True" \
    -o "boost/*:without_cobalt=True" \
    -o "boost/*:without_context=True" \
    -o "boost/*:without_contract=True" \
    -o "boost/*:without_coroutine=True" \
    -o "boost/*:without_date_time=True" \
    -o "boost/*:without_fiber=True" \
    -o "boost/*:without_filesystem=True" \
    -o "boost/*:without_graph=True" \
    -o "boost/*:without_graph_parallel=True" \
    -o "boost/*:without_iostreams=True" \
    -o "boost/*:without_json=True" \
    -o "boost/*:without_locale=True" \
    -o "boost/*:without_log=True" \
    -o "boost/*:without_math=True" \
    -o "boost/*:without_mpi=True" \
    -o "boost/*:without_nowide=True" \
    -o "boost/*:without_process=True" \
    -o "boost/*:without_python=True" \
    -o "boost/*:without_random=True" \
    -o "boost/*:without_stacktrace=True" \
    -o "boost/*:without_serialization=True" \
    -o "boost/*:without_thread=True" \
    -o "boost/*:without_timer=True" \
    -o "boost/*:without_type_erasure=True" \
    -o "boost/*:without_url=True" \
    -o "boost/*:without_wave=True" \
    -o "boost/*:zlib=False" \
    -o "openssl/*:no_apps=True" \
    -o "openssl/*:no_zlib=True" \
    -s "botan/*:compiler.cppstd=20" \
    {{args}} \
    {{workspace}}

_conan-host cmd build_type version shared: detect-host export \
  (_conan cmd "--version" version \
          "-s" "pichi/*:build_type=" + build_type \
          "-o" "shared=" + shared \
          if os() == "linux" { \
            "-o pichi/*:tls_fingerprint=True -o pichi/*:transparent=iptables" \
          } else if os() =~ "^(freebsd|macos)$" { \
            "-o pichi/*:tls_fingerprint=True -o pichi/*:transparent=pf" \
            } else if os() == "windows" { \
            if shared == "True" { \
              "-s compiler.runtime=dynamic" \
            } else { \
              "-s compiler.runtime=static" \
            } \
          } else { \
            "" \
          })

build version="latest" build_type="Release" shared="False": \
  (_conan-host "create" build_type version shared)

[unix]
dev build_type="Debug" shared="False" version="latest": \
  (_conan-host "install" build_type version shared)

[windows]
dev build_type="RelWithDebInfo" version="latest" shared="False": \
  (_conan-host "install" build_type version shared)

_conan-cross cmd build_type version *extra_args: \
  (_conan cmd \
          "--version" version \
          "-s" "pichi/*:build_type=" + build_type \
          "-o" "*:shared=False" \
          "-o" "pichi/*:tls_fingerprint=False" \
          "-o" "pichi/*:transparent=none" \
          "-o" "pichi/*:build_server=False" \
          "-o" "pichi/*:build_test=False" \
          "-o" "boost/*:without_atomic=True" \
          "-o" "boost/*:without_exception=True" \
          "-o" "boost/*:without_program_options=True" \
          "-o" "boost/*:without_system=True" \
          "-o" "boost/*:without_test=True" \
          extra_args)

build-android version="latest" ndk_ver="r29" arch="armv8" build_type="Release": \
  export \
  (detect-android ndk_ver) \
  (_conan-cross "create" build_type version "-s" "arch=" + arch "-pr" "android")

dev-android build_type="Debug" ndk_ver="r29" arch="armv8" version="latest": \
  export \
  (detect-android ndk_ver) \
  (_conan-cross "install" build_type version "-s" "arch=" + arch "-pr" "android")

[macos]
_conan-ios cmd sdk version build_type: export detect-host
  #!/bin/sh
  set -eu

  host_arch="{{ if arch() == 'x86_64' { 'x86_64' } else { 'armv8' } }}"
  case "{{sdk}}" in
    "iphoneos")
      os="iOS"
      arch="armv8"
      ;;
    "iphonesimulator")
      os="iOS"
      arch="$host_arch"
      ;;
    "appletvos")
      os="tvOS"
      arch="armv8"
      ;;
    "appletvsimulator")
      os="tvOS"
      arch="$host_arch"
      ;;
    "watchos")
      os="watchOS"
      arch="armv8"
      ;;
    "watchsimulator")
      os="watchOS"
      arch="$host_arch"
      ;;
    *)
      echo "Unsupported SDK: {{sdk}}"
      exit 1
      ;;
  esac

  ver=`xcrun --sdk "{{sdk}}" --show-sdk-version`
  [ -n "${ver}" ]

  {{just}} _conan-cross "{{cmd}}" "{{build_type}}" "{{version}}" \
    -s "os=${os}" -s "arch=${arch}" -s "os.sdk={{sdk}}" -s "os.version=${ver}"

[macos]
build-ios version="latest" sdk="iphoneos" build_type="Release": \
  (_conan-ios "create" sdk version build_type)

[macos]
dev-ios build_type="Debug" sdk="iphoneos" version="latest": \
  (_conan-ios "install" sdk version build_type)

[unix]
_prepare version artifact:
  #!/bin/sh
  set -eu

  recipe="pichi/{{version}}"
  pkg=`conan list -f json "${recipe}:*" | \
    jq -r "first(.\"Local Cache\".\"${recipe}\".revisions[]) | .packages | keys[0]" 2>/dev/null | \
    grep -v '^null$'`

  cp -f "$(conan cache path ${recipe}:${pkg})/bin/pichi" "{{artifact}}"
  strip "{{artifact}}"

[windows]
_prepare version artifact:
  #!powershell.exe

  $recipe = "pichi/{{version}}"
  $json = conan list -f json "${recipe}:*" 2>$null | ConvertFrom-Json
  $json = $json."Local Cache".$recipe.revisions | Select-Object -First 1
  $rev = ($json.PSObject.Properties | Select-Object -First 1).Name
  $pkg = ($json.$rev.packages.PSObject.Properties | Select-Object -First 1).Name
  $exe = Join-Path $(conan cache path "${recipe}:${pkg}") "bin\pichi.exe"
  Copy-Item -Force $exe "{{artifact}}"
