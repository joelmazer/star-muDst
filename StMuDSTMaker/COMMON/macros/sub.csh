#! /afs/rhic/star/users/laue/public/tcsh -f

# StRoot/StMuDstMaker/macros/sub.csh MuDst/centra/ReversedFullField/runs

set inputDir = $1
set dataSet =  st_physics
set maxJobs = 1000

@ i=0
@ s=0
foreach f (`find $inputDir -name '*.event.root' | grep $dataSet | grep -v BadFile`)
@ i++
  set baseName = `basename $f .event.root`
  set dirName = `dirname $f`  
  set log = $baseName\.log
  set outputDir = $dirName:s/MuDst/MuDST/
  set checkfile = $outputDir/$baseName\.MuDst\.root
  set outfile = /dev/null

# $outDir $checkfile

  if (-e $baseName.out) rm $baseName.out  
  if (-e $checkfile) then
    echo " *** $i $s   $checkfile file already exist ***"
  else 
    echo " *** $i $s   $checkfile ***"
    set user = `/sbin/fuser -u $f`
    echo $user
    if ($user == "") then
      @ s++
      bsub  -u laue -q star_cas -L ~/public/tcsh -o $outfile -e $outfile root4star -q -b StRoot/StMuDSTMaker/COMMON/macros/StMuDstMaker.C\(10,\"-\",\"$f:q\",\"$outputDir:q\"\) >& /dev/null
#      echo "submitted"
    endif
  endif
  if ($s > $maxJobs ) break;  
end 
bjobs
      
