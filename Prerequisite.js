prerequisites.js
name: 'Hello World'
description: 'MoneyMan573 <Michael Glenn>
  who-to-greet:  # id of input
    description: 'Who to greet'
    required: true
    default: 'World'
    description: "Random number"
    value: ${{ steps.random-number-generator.outputs.random-number }}
  using: "composite"
  jobs:
    - run: echo Hello ${{ inputs.who-to-greet }}.
      shell: bash
    - id: random-number-generator
      run: echo "random-number=$(echo $RANDOM)" >> $GITHUB_OUTPUT
      shell: bash
    - run: echo "${{ github.action_path }}" >> $GITHUB_PATH
      shell: bash
    - run: goodbye.sh
      shell: bash
