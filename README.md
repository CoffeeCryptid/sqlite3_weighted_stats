# sqlite3_weighted_stats
SQLite3 Weighted Summary Statistics Extension

Provides weighted mean, weighted variance and weighted standard deviation functions for sqlite3. The code was largely lifted from [Liam Healy's sqlite3 extension functions](https://www.sqlite.org/contrib) and adjusted. Calculations are based on [notes on incremental calculation of weighted mean and variance by Tony Finch](https://fanf2.user.srcf.net/hermes/doc/antiforgery/stats.pdf).

## FAQ

### How do I compile this?
gcc -fPIC -lm -shared sqlite\_wtd\_stats.c -o sqlite\_wtd\_stats.so  
on linux.

### How do I load the functions into SQLite?
SELECT load_extension('sqlite\_wtd\_stats');

### How do I use the functions?
- SELECT wtd_mean(x, weight) FROM table;  
- SELECT wtd_var(x, weight) FROM table;  
- SELECT wtd_sd(x, weight) FROM table;  

### Is this tested?
Not really.