translate([0,-63,70])rotate([20,0,0]){
    cube([10,10,150], center=true);
}
translate([0,40,0]){
    cube([10,10,15], center=true);
}
translate([0,63,70])rotate([-20,0,0]){
    cube([10,10,150], center=true);
}
translate([0,-40,0]){
    cube([10,10,15], center=true);
}
cube([10,70,10], center=true);
difference(){
translate([0,0,140])rotate([90,0,0])cylinder(h=182,d=30, center=true);
translate([0,0,140])rotate([90,0,0])cylinder(h=200,d=20, center=true);
    translate([0,0,190])rotate([90,0,0])cube([40,100,220], center=true);
    translate([0,0,140])rotate([0,90,0])cylinder(h=40,d=158, center=true);
}
