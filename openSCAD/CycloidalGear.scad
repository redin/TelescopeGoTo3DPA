include <lib-cyclogearprofiles.scad>
include <MCAD/stepper.scad>

r=0.7;

n=25;

centerdistance2 = rollingradius(r,n+1)-rollingradius(r,n); // r*(+2*n1b-2*n2b);
  dx2 = centerdistance2*cos(0);
  dy2 = centerdistance2*sin(0);

//disk
translate([dx2,dy2,0])linear_extrude(height = 7.5, center = true)difference(){
    cyclogearprofile(r,(n),24,1);
    circle(d=22, $fa=1);
    rotate([0,0,0])translate([28,0,0])circle(d=8, $fs=0.1);
    rotate([0,0,120])translate([28,0,0])circle(d=8, $fs=0.1);
    rotate([0,0,240])translate([28,0,0])circle(d=8, $fs=0.1);
}

//drive bearing
translate([dx2,dy2,0])color("gray")linear_extrude(height = 7, center = true)difference(){
    circle(d=21.9, $fa=1);
    circle(d=8, $fs=0.1);
}

//drive shaft
color("silver")translate([0,0,2])cylinder(d=5, h=30, center=true,$fs=0.1);

//rollers
//for (a = [0:13.846153846:360]) rotate([0,0,a-3.5]) translate([37.1,0,0])
//color("white")cylinder(d=4, h=10, center=true,$fs=0.1);

//color("purple")translate([-35.35,11,0])cylinder(d=4, h=10, center=true,$fs=0.1);

//Eccentric
color("red")difference(){
    translate([dx2,dy2,0])cylinder(d=7.99, h=8, center=true,$fs=0.1);
    translate([0,0,2])cylinder(d=5, h=22, center=true,$fs=0.1);
}

//TOP
translate([0,0,-8])color("cyan")difference(){
    cylinder(d=100,h=7, center=true,$fs=0.1, $fa=0.1 );
    cylinder(d=22,h=7.1, center=true,$fs=0.1, $fa=0.1 );
    for (a = [0:45:360]) rotate([0,0,a]) translate([47.5,0,0]) cylinder(d=3, h=20, center=true,$fs=0.1);
        
    for (a = [45:90:360]) rotate([0,0,a]) translate([21.95,0,0]) cylinder(d=4.05, h=20, center=true,$fs=0.1);
        
    translate([0,0,0]) rotate([0,180,0]) motor(Nema17, 1, dualAxis=false);
}

//Botton Cap
translate([0,0,8])difference(){
    cylinder(d=8,h=7, center=true,$fs=0.1 );
    cylinder(d=5,h=7.1, center=true, $fs=0.1 );
}
translate([0,0,8])color("blue")linear_extrude(height = 7, center = true)difference(){
    circle(d=21.9, $fa=1);
    circle(d=8, $fs=0.1);
}

//Base adapter
translate([0,0,17])union(){
    translate([0,0,16])difference(){
        cylinder(h=22, d=37 , center = true,$fa=1);
        translate([0,0,6.1])cylinder(h=10, d=9.7 , center = true,$fa=1);
    }
    cylinder(h=10, d=96 , center = true, $fa=1);
}

//Driven rotor
color("orange")union(){difference(){
    translate([0,0,8])linear_extrude(height = 8, center = true)difference(){
        circle(d=95, $fa=1);
        circle(d=22, $fs=0.1);
    }

    translate([0, 0, 3])rotate_extrude(convexity = 20,$fn=100)translate([42, 0, 0])circle(r = 2.4);

    rotate([0,0,0])translate([28,0,8])
        cylinder(d=2.8, h=9, center=true,$fs=0.1);
    rotate([0,0,120])translate([28,0,8])
        cylinder(d=2.8, h=9, center=true,$fs=0.1);
    rotate([0,0,240])translate([28,0,8])
        cylinder(d=2.8, h=9, center=true,$fs=0.1);
    }

    rotate([0,0,0])translate([28,0,0])difference(){
        cylinder(d=5, h=9, center=true,$fs=0.1);
        cylinder(d=2.8, h=10, center=true,$fs=0.1);
    }
        
    rotate([0,0,120])translate([28,0,0])difference(){
        cylinder(d=5, h=9, center=true,$fs=0.1);   
        cylinder(d=2.8, h=10, center=true,$fs=0.1);
    }
        
    rotate([0,0,240])translate([28,0,0])difference(){
        cylinder(d=5, h=9, center=true,$fs=0.1);
        cylinder(d=2.8, h=10, center=true,$fs=0.1);
    }
}

//Stator outer ring
//translate([0,0,8])color("gray")difference(){
//    linear_extrude(height = 8, center = true) difference(){
//        circle(r=60, $fa=1);
//        circle(r=40.1, $fa=1);
//        
//    }
//    rotate_extrude(convexity = 20,$fn=100)
//translate([40, 0, 0])
//circle(r = 2.5);
//translate([40,0,4])
//    cylinder(d=5, h=8, center=true,$fs=0.1);
//    translate([55,0,-5.1])cylinder(r=1.5, h=11, $fs=0.1);
//    translate([-55,0,-5.1])cylinder(r=1.5, h=11, $fs=0.1);
//    translate([0,-55,-5.1])cylinder(r=1.5, h=11, $fs=0.1);
//    translate([0,55,-5.1])cylinder(r=1.5, h=11, $fs=0.1);
//    
//}

union(){
    for (a = [0:8:360]) rotate([0,0,a]) translate([42,0,3]) sphere(d=4.7, $fs=0.1);   
color("green")difference(){
    linear_extrude(height = 7.5, center = true) difference(){
        circle(r=50, $fa=1);
        //circle(r=39, $fa=1);
        cyclogearprofile(r,n+1,24, 1);
    }
    for (a = [0:45:360]) rotate([0,0,a]) translate([47.5,0,0]) cylinder(d=3, h=20, center=true,$fs=0.1);
    translate([0, 0, 3])rotate_extrude(convexity = 20,$fn=100)translate([42, 0, 0])circle(r = 2.4);  
}
}

//translate([0,0,-9.01])difference(){
//        cylinder(r=60, h=4, $fa=1);
//        translate([0,0,-0.1])cylinder(r=11, h=6,  $fs=0.1);
//        translate([15.5,15.5,-0.1])cylinder(r=1.5, h=6, $fs=0.1);
//        translate([-15.5,-15.5,-0.1])cylinder(r=1.5, h=6, $fs=0.1);
//        translate([15.5,-15.5,-0.1])cylinder(r=1.5, h=6, $fs=0.1);
//        translate([-15.5,15.5,-0.1])cylinder(r=1.5, h=6, $fs=0.1);
//        
//        translate([15.5,15.5, 1.21])cylinder(d=5.6, h=2.8, $fs=0.1);
//        translate([-15.5,-15.5, 1.21])cylinder(d=5.6, h=2.8, $fs=0.1);
//        translate([15.5,-15.5, 1.21])cylinder(d=5.6, h=2.8, $fs=0.1);
//        translate([-15.5,15.5, 1.21])cylinder(d=5.6, h=2.8, $fs=0.1);
//        
//        translate([55,0,-5.1])cylinder(r=1.5, h=11, $fs=0.1);
//        translate([-55,0,-0.1])cylinder(r=1.5, h=6, $fs=0.1);
//        translate([0,-55,-0.1])cylinder(r=1.5, h=6, $fs=0.1);
//        translate([0,55,-0.1])cylinder(r=1.5, h=6, $fs=0.1);
//    }