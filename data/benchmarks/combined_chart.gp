# Combined Timing Data Plot
# Plots output of combine_task2.py as a line graph with chunksize on x-axis.
#
# Usage:
#   cat task_2/combined_N_8000-RANK_10.dat | gnuplot -e "title='...'" combined_chart.gp
#
# Optional mode variable: "all" (default), "total", "avg", "var"
#   cat ... | gnuplot -e "title='...'; mode='avg'" combined_chart.gp

if (!exists("mode")) mode = "all"

set title title
set title font "Sans Bold, 24"

set xlabel "Chunk Size" font ",18"
set ylabel "Time (seconds)" font ",18"

set xtics font ",12"
set ytics font ",12"

set logscale x 10
set format x "10^{%T}"

set key outside bottom right opaque
set key font ",16"
set key spacing 1
set key reverse
set key width 2
set key Left
set key samplen 4
set key offset -5, 0

set lmargin 12
set rmargin 45
set tmargin 5
set bmargin 5

set xtics nomirror
set ytics nomirror
set grid xtics ytics lc rgb "#cccccc" lw 1

# Solid lines for runtime values
set style line 1 lc rgb "#2c3e50" lw 2.5 pt 7  ps 1.2
set style line 2 lc rgb "#2980b9" lw 2   pt 5  ps 1.0
set style line 3 lc rgb "#e67e22" lw 2   pt 9  ps 1.0
set style line 4 lc rgb "#c0392b" lw 2   pt 13 ps 1.0
# Dashed lines for variance values
set style line 5 lc rgb "#2980b9" lw 1.5 pt 4  ps 0.8 dt 2
set style line 6 lc rgb "#e67e22" lw 1.5 pt 8  ps 0.8 dt 2
set style line 7 lc rgb "#c0392b" lw 1.5 pt 12 ps 0.8 dt 2

set datafile separator "\t"
set table $data
plot '/dev/stdin' using 1:2:3:4:5:6:7:8 with table
unset table
set datafile separator whitespace

if (mode eq "total") {
    plot $data using 1:2 with linespoints ls 1 title "Total Runtime"
} else if (mode eq "avg") {
    plot $data using 1:3 with linespoints ls 2 title "Avg Work", \
         ''    using 1:4 with linespoints ls 3 title "Avg Wait", \
         ''    using 1:5 with linespoints ls 4 title "Avg Comm"
} else if (mode eq "var") {
    plot $data using 1:6 with linespoints ls 5 title "Var Work", \
         ''    using 1:7 with linespoints ls 6 title "Var Wait", \
         ''    using 1:8 with linespoints ls 7 title "Var Comm"
} else {
    plot $data using 1:2 with linespoints ls 1 title "Total Runtime", \
         ''    using 1:3 with linespoints ls 2 title "Avg Work", \
         ''    using 1:4 with linespoints ls 3 title "Avg Wait", \
         ''    using 1:5 with linespoints ls 4 title "Avg Comm", \
         ''    using 1:6 with linespoints ls 5 title "Var Work", \
         ''    using 1:7 with linespoints ls 6 title "Var Wait", \
         ''    using 1:8 with linespoints ls 7 title "Var Comm"
}

pause mouse close
