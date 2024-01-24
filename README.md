# Live Reload C Program

Use your favorite compiler, then run `./your_program` followed by your command. For example, `./main go run main.go` will run `main.go`. It watches for changes in your current directory and re-executes the `go run main.go` command whenever changes are detected.
It needs directory ignore options(will add later or not), cleanup and more testing. As of now it ignores `.git` and `live_reload.dSYM` from `lldb`
To ignore files/directories add them after --ignore. For example, `./main go run main.go --ignore file1 dir1 file2 dir2`

![live_reload](https://github.com/Rogerpeke97/live_reload/assets/65107071/980bc752-e100-4f3f-924f-7da15190d01b)

PS: Will definitely fail on windows 
