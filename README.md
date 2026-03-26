# NAACLI
Notes as a Command Line Interface is a cli tool for creating notes, todos, and other reminders inside of your projects. I wrote the tool because I often have quick things that I want to note down and I do a lot of work from the terminal. Could I have just downloaded this from the internet? Yes. But this was more fun and enriching.

# INSTALLATION
1. ```git clone https://github.com/jbic9832/naacli```
2. Navigate to ./naacli and run ```make```
3. Add the resulting executable to your PATH

# USAGE
- Add item
    - ```naacli add [category] [content]```

- List all items
    - ```naacli list```

- List all items in a category
    - ```naacli list [category]```

NAACLI will create a notes.db file in what ever directory you run it from.

I recommend that you commit your "notes.db" file with all of your code so that your notes can persist across all machines that you work on. If you do this __MAKE SURE YOU DON'T ADD SENSITIVE INFORMATION!!!__ Otherwise you can just add "notes.db" to your gitignore.

