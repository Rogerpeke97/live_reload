# Live Reload C Program

Use your favorite compiler, then run `./your_program` followed by your command. For example, `./main go run main.go` will run `main.go`. It watches for changes in your current directory and re-executes the `go run main.go` command whenever changes are detected.
Needs directory ignore options. As of now it ignore `.git` and `live_reload.dSYM` from `lldb`

PS: Will not work on windows :)

