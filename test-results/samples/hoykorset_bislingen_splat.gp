set grid
set yrange [169.175 to 767.949]
set y2range [-37.876 to 560.898]
set xrange [-0.5 to 28.968]
set encoding iso_8859_1
set term png
set title "SPLAT! Height Profile Between LA5MR_Bislingen and LA5MR_Hoykorset (359.45° azimuth)"
set xlabel "Distance Between LA5MR_Bislingen and LA5MR_Hoykorset (27.61 kilometers)"
set ylabel "Normalized Height Referenced To LOS Path Between\nLA5MR_Bislingen and LA5MR_Hoykorset (meters)"
set output "height.png"
plot "profile.gp" title "Point-to-Point Profile" with lines, "reference.gp" title "Line Of Sight Path" with lines, "curvature.gp" axes x1y2 title "Earth's Curvature Contour" with lines
