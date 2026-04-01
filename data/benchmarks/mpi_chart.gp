# MPI Benchmark Plot


unset title

set style data histograms
set style histogram rowstacked
set style fill solid border -1
set boxwidth 0.6

set xlabel "Time (seconds)" font ",18"
set ylabel "Rank" font ",18"

set xtics font ",12"
set ytics font ",12"

set key outside bottom right opaque
set key font ",16"
set key spacing 1.5
set key reverse
set key width 2
set key Left
set key samplen 4
set key offset 0, 0

set lmargin 10
set rmargin 36
set tmargin 5
set bmargin 5


stats '/dev/stdin'
N = STATS_records
set yrange [N - 0.5 : -0.5]
set ytics 1

set xtics nomirror
set grid xtics lc rgb "#cccccc" lw 1

plot '/dev/stdin' using 2:1:(0):($2):(($1)-0.4):(($1)+0.4) \
         with boxxyerror title "Computation", \
     ""  using ($2+$3):1:(($2)):($2+$3):(($1)-0.4):(($1)+0.4) \
         with boxxyerror title "Waiting", \
     ""  using ($2+$3+$4):1:($2+$3):($2+$3+$4):(($1)-0.4):(($1)+0.4) \
         with boxxyerror title "Communication"

