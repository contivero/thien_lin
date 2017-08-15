# Try distributing and recovering an image
../bin/bmpsss -d --secret Albert.bmp -w 300 -h 300 -k 8 --dir unpermuted_300x300
mkdir -p shades-tmp
mv shadow*.bmp shades-tmp
mkdir -p outputs
../bin/bmpsss -r --secret outputs/distributed_and_recovered.bmp -k 8 -w 300 -h 300 --dir shades-tmp

# Try recovering different sized images.
../bin/bmpsss -r --secret outputs/output1.bmp -k 8 -w 300 -h 300 --dir unpermuted_300x300
../bin/bmpsss -r --secret outputs/output2.bmp -k 8 -w 450 -h 300 --dir unpermuted_450x300
../bin/bmpsss -r --secret outputs/output3.bmp -k 8 -w 300 -h 450 --dir unpermuted_300x450
