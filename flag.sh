#!/bin/bash

banned_words=("phishing" "hacking" "sploit")

contains_banned_words() {
    local input="$1"
    for word in "${banned_words[@]}"; do
        if [[ "$input" == *"$word"* ]]; then
            return 0
        fi
    done
    return 1
}

search_query="your_search_query"
api_url="https://api.github.com/search/repositories?q=$search_query"

access_token="your access token"

search_results=$(curl -H "Authorization: token $access_token" \
                    -H "Accept: application/vnd.github.v3+json" \
                    "$api_url")

# Extract repository names from search results
repo_names=($(echo "$search_results" | jq -r '.items[].name'))

for repo_name in "${repo_names[@]}"; do
    if contains_banned_words "$repo_name"; then
        echo "Blocked repository name detected: $repo_name"
        echo "Terminating process..."
        kill $$
    fi
done

echo "Repository names:"
for repo_name in "${repo_names[@]}"; do
    echo "$repo_name"
done

