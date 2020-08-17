#! /bin/bash
cd /home/ca/Weather/script
../wstation ../WSRustadgrenda.db -s
echo "Getting data..."
../wstation ../WSRustadgrenda.db -xd=1  -xe=152 -xr=36.3 -xh=../www/latest.htm  > ../www/ws_day.txt
../wstation ../WSRustadgrenda.db -xd=7  -xe=152 -xr=36.3                        > ../www/ws_week.txt
../wstation ../WSRustadgrenda.db -xd=31 -xe=152 -xr=36.3                        > ../www/ws_month.txt
echo "Plotting..."
gnuplot gnuplot-ws.txt
echo "ftp..."
ftp -n < ftp_upload.txt
echo "Finished!"