$fa = .3;	// minimum angle 
$fs = .3;	// minimum size 
rotate([180,0,0]){
    difference(){
        translate([0,0,8])cylinder(h=2,d1=100,d2=105);
        cylinder(h=19,d=8.3);
        translate([40,0,9]){
            cube([5,1,5]);
        }
        translate([0,40,0]){
            cube([10,10,30], center=true);
        }
        translate([0,-40,0]){
            cube([10,10,30], center=true);
        }
    }
    difference(){
        translate([0,0,-2])cylinder(h=2,d1=105,d2=100);
        cylinder(h=19,d=8.3);
        translate([40,0,9]){
            cube([5,1,5]);
        }
        translate([0,40,0]){
            cube([10,10,30], center=true);
        }
        translate([0,-40,0]){
            cube([10,10,30], center=true);
        }
        cylinder(h=10,d=22.3,center=true);
        cylinder(h=9,d=8.3);
    }
    difference(){
        cylinder(h=8,d=100);
        for(i=[0:5:360]){
            translate([cos(i)*49.6,sin(i)*49.6]){
                    cylinder(h=20,d=2.5,center=true);
            }
        };
        cylinder(h=10,d=22.3,center=true);
        cylinder(h=9,d=8.3);
//        difference(){
//            cylinder(h=3,d=60, center=true);
//            cylinder(h=3,d=58.4, center=true);
//        }
        translate([0,40,0]){
            cube([10,10,18], center=true);
        }
        translate([0,-40,0]){
            cube([10,10,18], center=true);
        }
    }
}