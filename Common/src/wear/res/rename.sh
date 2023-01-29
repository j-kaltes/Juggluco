#name=blue.png
#colors="green yellow red white gray"
colors=blue
for col in $colors
do
name=$col.png
echo $name
for i in *mipmap*/$name
do 
 mv $i ${i/mipmap/drawable} 
done
 rm  mipmap*/$name

done
