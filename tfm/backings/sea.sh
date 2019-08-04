
# 1824 - (32+64*3)

yutyut /mnt/c/ireart/wata2.cel seashell.1 1600

for file in water/*.cel
do
	echo $file
	yumyum $file seashell.2 224
	cat seashell.1 seashell.2 > $file
done
