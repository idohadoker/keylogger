cmd_/home/idohadoker/test/Module.symvers := sed 's/\.ko$$/\.o/' /home/idohadoker/test/modules.order | scripts/mod/modpost -m -a  -o /home/idohadoker/test/Module.symvers -e -i Module.symvers   -T -
