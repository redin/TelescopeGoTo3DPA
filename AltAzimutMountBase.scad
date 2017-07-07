$fa = .3;	// minimum angle 
$fs = .3;	// minimum size 
rotate([180,0,0]){
    difference(){
        cylinder(h=8,d=100);
        for(i=[0:5:360]){
            translate([cos(i)*49.6,sin(i)*49.6]){
                    cylinder(h=20,d=2.5,center=true);
            }
        };
        cylinder(h=13,d=22.3,center=true);
        cylinder(h=9,d=8.3);
        difference(){
            cylinder(h=3,d=60, center=true);
            cylinder(h=3,d=58.4, center=true);
        }
        translate([40,0,7]){
            cube([5,1,5]);
        }
        translate([0,40,0]){
            cube([10,10,18], center=true);
        }
        translate([0,-40,0]){
            cube([10,10,18], center=true);
        }
    }
}