$fa = 0.1;
$fs = 0.1;
difference(){
    translate([0,10,-4]) cube([50,10,3]);
    translate([15,15,-5])cylinder(d=4,h=100);
    translate([35,15,-5])cylinder(d=4,h=100);
}

difference(){
    translate([0,80,-4]) cube([50,10,3]);
    translate([15,85,-5])cylinder(d=4,h=100);
    translate([35,85,-5])cylinder(d=4,h=100);
}

translate([10,10,-4])rotate([0,0,90]) cube([70,10,3]);
translate([50,10,-4])rotate([0,0,90]) cube([70,10,3]);

translate([60,40,-4])rotate([0,0,90]) cube([20,10,3]);
translate([-5,40,-4])rotate([0,0,90]) cube([20,5,8]);
translate([-20,50,0])rotate([0,90,0])cylinder(d=7.9,h=10);
translate([0,40,-4])rotate([0,0,90]) cube([20,10,3]);
translate([60,40,-4])rotate([0,0,90]) cube([20,5,8]);
translate([60,50,0])rotate([0,90,0])cylinder(d=7.9,h=10);



difference(){
    cube([50,100,12]);
    translate([15,15,-1])cylinder(d=4,h=100);
    translate([35,15,-1])cylinder(d=4,h=100);
    translate([5,10,7.1])cube([40,10,5]);
    
    translate([25,35,-1])cylinder(d=4,h=100);
    translate([25,50,-1])cylinder(d=4,h=100);
    translate([25,65,-1])cylinder(d=4,h=100);
    
    translate([15,85,-1])cylinder(d=4,h=100);
    translate([35,85,-1])cylinder(d=4,h=100);
    translate([5,80,7.1])cube([40,10,5]);
    
    translate([15,35,-1])cylinder(d=4,h=100);
    translate([35,35,-1])cylinder(d=4,h=100);
    translate([15,50,-1])cylinder(d=4,h=100);
    translate([35,50,-1])cylinder(d=4,h=100);
    translate([15,65,-1])cylinder(d=4,h=100);
    translate([35,65,-1])cylinder(d=4,h=100);
}
translate([0,100,3])rotate([90,0,0])cylinder(d=3,h=100);
translate([50,100,3])rotate([90,0,0])cylinder(d=3,h=100);

translate([0,100,9])rotate([90,0,0])cylinder(d=5,h=100);
translate([50,100,9])rotate([90,0,0])cylinder(d=5,h=100);
translate([-2.5,100,0]) cube([55,5,15]);
