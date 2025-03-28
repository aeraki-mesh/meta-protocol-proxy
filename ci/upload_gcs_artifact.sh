#!/bin/bash

set -e -o pipefail

if [[ -z "${GCS_ARTIFACT_BUCKET}" ]]; then
    echo "Artifact bucket is not set, not uploading artifacts."
    exit 1
fi

if [[ -z "${GCP_SERVICE_ACCOUNT_KEY}" ]]; then
    echo "GCP key is not set, not uploading artifacts."
    exit 1
fi

read -ra BAZEL_STARTUP_OPTIONS <<< "${BAZEL_STARTUP_OPTION_LIST:-}"
read -ra BAZEL_BUILD_OPTIONS <<< "${BAZEL_BUILD_OPTION_LIST:-}"

remove_key () {
    rm -rf "$KEYFILE"
}

trap remove_key EXIT

# Fail when service account key is not specified
KEYFILE="$(mktemp)"
bash -c 'echo ${GCP_SERVICE_ACCOUNT_KEY}' | base64 --decode > "$KEYFILE"

cat <<EOF > ~/.boto
[Credentials]
gs_service_key_file=${KEYFILE}
EOF

SOURCE_DIRECTORY="$1"
TARGET_SUFFIX="$2"

if [ ! -d "${SOURCE_DIRECTORY}" ]; then
    echo "ERROR: ${SOURCE_DIRECTORY} is not found."
    exit 1
fi

if [[ "$BUILD_REASON" == "PullRequest" ]] || [[ "$TARGET_SUFFIX" == "docs" ]] || [[ "$TARGET_SUFFIX" == "docker" ]]; then
    # upload to the last commit sha (first 7 chars), either
    # - docs/docker build on main, eg docs
    #      -> https://storage.googleapis.com/envoy-postsubmit/$UPLOAD_PATH/docs/envoy-docs-rst.tar.gz
    # - PR build (commit sha from the developers branch)
    #      -> https://storage.googleapis.com/envoy-pr/$UPLOAD_PATH/$TARGET_SUFFIX
    UPLOAD_PATH="$(git rev-parse HEAD | head -c7)"
else
    UPLOAD_PATH="${SYSTEM_PULLREQUEST_PULLREQUESTNUMBER:-${BUILD_SOURCEBRANCHNAME}}"
fi

GCS_LOCATION="${GCS_ARTIFACT_BUCKET}/${UPLOAD_PATH}/${TARGET_SUFFIX}"

echo "Uploading to gs://${GCS_LOCATION} ..."
bazel "${BAZEL_STARTUP_OPTIONS[@]}" run "${BAZEL_BUILD_OPTIONS[@]}" \
      //tools/gsutil \
      -- -mq rsync \
         -dr "${SOURCE_DIRECTORY}" \
         "gs://${GCS_LOCATION}"

# For PR uploads, add a redirect `PR_NUMBER` -> `COMMIT_SHA`
if [[ "$BUILD_REASON" == "PullRequest" ]]; then
    REDIRECT_PATH="${SYSTEM_PULLREQUEST_PULLREQUESTNUMBER:-${BUILD_SOURCEBRANCHNAME}}"
    TMP_REDIRECT="/tmp/redirect/${REDIRECT_PATH}/${TARGET_SUFFIX}"
    mkdir -p "$TMP_REDIRECT"
    echo "<meta http-equiv=\"refresh\" content=\"0; URL='https://storage.googleapis.com/${GCS_LOCATION}/index.html'\" />" \
         >  "${TMP_REDIRECT}/index.html"
    GCS_REDIRECT="${GCS_ARTIFACT_BUCKET}/${REDIRECT_PATH}/${TARGET_SUFFIX}"
    echo "Uploading redirect to gs://${GCS_REDIRECT} ..."
    bazel "${BAZEL_STARTUP_OPTIONS[@]}" run "${BAZEL_BUILD_OPTIONS[@]}" \
          //tools/gsutil \
          -- -h "Cache-Control:no-cache,max-age=0" \
             -mq rsync \
             -dr "${TMP_REDIRECT}" \
             "gs://${GCS_REDIRECT}"
fi

if [[ "${COVERAGE_FAILED}" -eq 1 ]]; then
    echo "##vso[task.logissue type=error]Coverage failed, check artifact at: https://storage.googleapis.com/${GCS_LOCATION}/index.html"
fi

echo "Artifacts uploaded to: https://storage.googleapis.com/${GCS_LOCATION}/index.html"
