#!/bin/bash

CHANGELOG_FILE=CHANGELOG.md

read_changelog_latest_version() {
    VERSION_LINE=$(grep -m 1 "^##" $CHANGELOG_FILE | grep -Po '#*\sVersion\s\K.*')
    VERSION_DATE=$(echo $VERSION_LINE | grep -Po '.*\s\(\K.*(?=\))')
    VERSION=$(echo $VERSION_LINE | grep -Po '[a-zA-Z0-9\.]*(?=\s.*)')
    
    echo "$VERSION"
}

read_changelog_body() {
   read -d '' -a VERSION_LINE_NUMBERS < <(grep -n -P "^##.*" $CHANGELOG_FILE | head -n 2 | cut -f1 -d:)
   PREVIOUS_VERSION_LINE_NUMBER=${VERSION_LINE_NUMBERS[1]}
   CURRENT_VERSION_LINE_NUMBER=${VERSION_LINE_NUMBERS[0]}
   
   CHANGELOG_BODY=$(cat $CHANGELOG_FILE | tail -n +$(($CURRENT_VERSION_LINE_NUMBER+1)) | head -n $(($PREVIOUS_VERSION_LINE_NUMBER - $CURRENT_VERSION_LINE_NUMBER - 1)))
   
   CHANGELOG_BODY=$(sed -e '/./,$!d' -e :a -e '/^\n*$/{$d;N;ba' -e '}' <<< $CHANGELOG_BODY) # Remove leading and trailing empty lines
   echo "$CHANGELOG_BODY"
}

PREVIOUS_RELEASE_COMMIT=$(git rev-list -n 1 tags/$PREVIOUS_RELEASE_TAG)

# For testing PREVIOUS_RELEASE_COMMIT=5427fe8f5a427e1baa1e6fb7d5d8cf938362765c

CHANGELOG_CHANGE_COMMIT=$(git log $PREVIOUS_RELEASE_COMMIT^...HEAD --pretty=%H -- $CHANGELOG_FILE)
CHANGELOG_CHANGE_COMMIT=${CHANGELOG_CHANGE_COMMIT##*$'\n'} # Is mostly gonna be equal to PREVIOUS_RELEASE_COMMIT

echo "Previous release commit: $CHANGELOG_CHANGE_COMMIT"

if [ -z "$CHANGELOG_CHANGE_COMMIT" ]
then
    echo "No changes made to Changelog file: '$CHANGELOG_FILE' since latest release '$PREVIOUS_RELEASE_COMMIT'"
    exit
fi
# Revert changelog file to get previous version info
git checkout $CHANGELOG_CHANGE_COMMIT $CHANGELOG_FILE 2> /dev/null || exit $?

PREVIOUS_VERSION=$(read_changelog_latest_version)

# Restore original changelog file
git restore --staged $CHANGELOG_FILE 2> /dev/null || exit $?
git restore $CHANGELOG_FILE 2> /dev/null || exit $?


CURRENT_VERSION=$(read_changelog_latest_version)

if [ "$PREVIOUS_VERSION" == "$CURRENT_VERSION" ];
then
    echo "Version number did not change"
    exit
fi


CHANGELOG_BODY=$(read_changelog_body)

echo "Body:"
echo "$CHANGELOG_BODY"
CHANGELOG_BODY_ESCAPED="${CHANGELOG_BODY//'%'/'%25'}"
CHANGELOG_BODY_ESCAPED="${CHANGELOG_BODY_ESCAPED//$'\n'/'%0A'}"
CHANGELOG_BODY_ESCAPED="${CHANGELOG_BODY_ESCAPED//$'\r'/'%0D'}"

echo "###############################################"

echo "::set-output name=RELEASE_VERSION::$CURRENT_VERSION"
echo "::set-output name=RELEASE_CHANGELOG::$CHANGELOG_BODY_ESCAPED"
echo "::set-output name=IS_RELEASE::True"