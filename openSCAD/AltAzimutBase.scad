$fa = .3;	// minimum angle 
$fs = .3;	// minimum size
difference(){
    translate([0,0,-6])cylinder(h=12,d=25);
    cylinder(h=15,d=8,center=true);
}
rotate([0,0,0]) translate([34,0,0]){
    difference(){ 
        union(){
            difference(){
                cube([56,16,12], center=true);
                rotate([0,0,0])translate([-3.5,0,3.5])screwshadow();
            }
            translate([22,0,4.5])cube([12,40,3],center=true);
        }
        translate([28,0,1])cylinder(h=15,d=25,center=true);
        translate([25,-16,1])cube([12,4,12],center=true);
        translate([25,16,1])cube([12,4,12],center=true);
    }
}
rotate([0,0,120]) translate([25,0,0]){
    difference(){
        cube([30,16,12], center=true);
        rotate([0,0,0])translate([5.5,0,3.5])screwshadow();
    }
}
rotate([0,0,240]) translate([25,0,0]){
    difference(){
        cube([30,16,12], center=true);
        rotate([0,0,0])translate([5.5,0,3.5])screwshadow();
    }
}

//screwshadow();

module screwshadow(){
    translate([3.5,0,3])cube([3.5,5.5,12], center=true);
    translate([0,0,0])cube([4,12,12], center=true);
    translate([-3.5,0,3])cube([3.5,5.5,12], center=true);
}