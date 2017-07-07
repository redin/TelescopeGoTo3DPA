$fa = .3;	// minimum angle 
$fs = .3;	// minimum size 
difference(){
    cylinder(h=8,d=25, center=true);
    cube([5.1,3.1,14], center=true);
    for(i=[0:40:360]){
            translate([cos(i)*12.1,sin(i)*12.1,0]){
                     cylinder(h=20,d=5,center=true);
            }
        };
    }