#include "main.h"

// Motor constructors
pros::Motor intake (-1, pros::v5::MotorGears::blue);  
pros::Motor ladybrown_Left (18, pros::v5::MotorGears::green);
pros::Motor ladybrown_Right (19, pros::v5::MotorGears::green);

// Pneumatic constructors
ez::Piston intakeLifter('A', false);            // Used EZ Template for toggle function
pros::adi::Pneumatics mogoL ('B', false);
pros::adi::Pneumatics mogoR ('C', false);
ez::Piston doinker('D', false);                 // Used EZ Template for toggle function

// Sensor constructors
pros::Imu imu(12);
pros::v5::Optical optical(10);

// Intake move function
void setIntake(int intakePower){                                                   
  intake.move(intakePower);
}

// Ladybrown move function
void setLB(int lbPower){
  ladybrown_Right.move(lbPower);
  ladybrown_Left.move(-lbPower);
}

// Simplify Ladybrown brake state
void setLBbrake(){
  ladybrown_Left.set_brake_mode(pros::E_MOTOR_BRAKE_HOLD);
  ladybrown_Right.set_brake_mode(pros::E_MOTOR_BRAKE_HOLD);
}

// Extend mogo function
void extendMogo(){
  mogoL.retract();
  mogoR.extend();
}

// Retract mogo function
void retractMogo(){
  mogoL.extend();
  mogoR.retract();
}

// Chassis constructor
ez::Drive chassis(
    
    {-11, -9, 6},
    {20, 7, -8}, 

    12,      // IMU Port
    3.25,  // Wheel Diameter
    450);   // Wheel RPM = cartridge * (motor gear / wheel gear)

/**
 * Runs initialization code. This occurs as soon as the program is started.
 *
 * All other competition modes are blocked by initialize; it is recommended
 * to keep execution time for this mode under a few seconds.
 */
void initialize() {

  pros::delay(500);  // Stop the user from doing anything while legacy ports configure

  // Configure your chassis controls
  chassis.opcontrol_curve_buttons_toggle(true);   // Enables modifying the controller curve with buttons on the joysticks
  chassis.opcontrol_drive_activebrake_set(0.0);   // Sets the active brake kP. We recommend ~2.  0 will disable.
  chassis.opcontrol_curve_default_set(0.0, 0.0);  // Defaults for curve.

  // Set default constants from autons.cpp
  default_constants();

  // Autonomous Selector using LLEMU
  ez::as::auton_selector.autons_add({
      {"Turn Test", turnTest},
      {"Blue Negative SAWP\n8 points total", blueSAWP},
      {"Red Negative SAWP\n8 points total", redSAWP},
      {"Skills", skills},
  });

  // Initialize chassis and auton selector
  chassis.initialize();
  ez::as::initialize();
  master.rumble(chassis.drive_imu_calibrated() ? "." : "---");

  extendMogo();
}

/**
 * Runs while the robot is in the disabled state of Field Management System or
 * the VEX Competition Switch, following either autonomous or opcontrol. When
 * the robot is enabled, this task will exit.
 */
void disabled() {
  // . . .
}

/**
 * Runs after initialize(), and before autonomous when connected to the Field
 * Management System or the VEX Competition Switch. This is intended for
 * competition-specific initialization routines, such as an autonomous selector
 * on the LCD.
 *
 * This task will exit when the robot is enabled and autonomous or opcontrol
 * starts.
 */
void competition_initialize() {
  // . . .
}

/**
 * Runs the user autonomous code. This function will be started in its own task
 * with the default priority and stack size whenever the robot is enabled via
 * the Field Management System or the VEX Competition Switch in the autonomous
 * mode. Alternatively, this function may be called in initialize or opcontrol
 * for non-competition testing purposes.
 *
 * If the robot is disabled or communications is lost, the autonomous task
 * will be stopped. Re-enabling the robot will restart the task, not re-start it
 * from where it left off.
 */
void autonomous() {
  chassis.pid_targets_reset();                // Resets PID targets to 0
  chassis.drive_imu_reset();                  // Reset gyro position to 0
  chassis.drive_sensor_reset();               // Reset drive sensors to 0
  chassis.odom_xyt_set(0_in, 0_in, 0_deg);    // Set the current position, you can start at a specific position with this
  chassis.drive_brake_set(MOTOR_BRAKE_HOLD);  // Set motors to hold.  This helps autonomous consistency

  /*
  Odometry and Pure Pursuit are not magic

  It is possible to get perfectly consistent results without tracking wheels,
  but it is also possible to have extremely inconsistent results without tracking wheels.
  When you don't use tracking wheels, you need to:
   - avoid wheel slip
   - avoid wheelies
   - avoid throwing momentum around (super harsh turns, like in the example below)
  You can do cool curved motions, but you have to give your robot the best chance
  to be consistent
  */

  ez::as::auton_selector.selected_auton_call();  // Calls selected auton from autonomous selector
}

/**
 * Simplifies printing tracker values to the brain screen
 */
void screen_print_tracker(ez::tracking_wheel *tracker, std::string name, int line) {
  std::string tracker_value = "", tracker_width = "";
  // Check if the tracker exists
  if (tracker != nullptr) {
    tracker_value = name + " tracker: " + util::to_string_with_precision(tracker->get());             // Make text for the tracker value
    tracker_width = "  width: " + util::to_string_with_precision(tracker->distance_to_center_get());  // Make text for the distance to center
  }
  ez::screen_print(tracker_value + tracker_width, line);  // Print final tracker text
}

/**
 * Ez screen task
 * Adding new pages here will let you view them during user control or autonomous
 * and will help you debug problems you're having
 */
void ez_screen_task() {
  while (true) {
    // Only run this when not connected to a competition switch
    if (!pros::competition::is_connected()) {
      // Blank page for odom debugging
      if (chassis.odom_enabled() && !chassis.pid_tuner_enabled()) {
        // If we're on the first blank page...
        if (ez::as::page_blank_is_on(0)) {
          // Display X, Y, and Theta
          ez::screen_print("x: " + util::to_string_with_precision(chassis.odom_x_get()) +
                               "\ny: " + util::to_string_with_precision(chassis.odom_y_get()) +
                               "\na: " + util::to_string_with_precision(chassis.odom_theta_get()),
                           1);  // Don't override the top Page line

          // Display all trackers that are being used
          screen_print_tracker(chassis.odom_tracker_left, "l", 4);
          screen_print_tracker(chassis.odom_tracker_right, "r", 5);
          screen_print_tracker(chassis.odom_tracker_back, "b", 6);
          screen_print_tracker(chassis.odom_tracker_front, "f", 7);
        }
      }
    }

    // Remove all blank pages when connected to a comp switch
    else {
      if (ez::as::page_blank_amount() > 0)
        ez::as::page_blank_remove_all();
    }

    pros::delay(ez::util::DELAY_TIME);
  }
}
pros::Task ezScreenTask(ez_screen_task);

/**
 * Gives you some extras to run in your opcontrol:
 * - run your autonomous routine in opcontrol by pressing DOWN and B
 *   - to prevent this from accidentally happening at a competition, this
 *     is only enabled when you're not connected to competition control.
 * - gives you a GUI to change your PID values live by pressing X
 */
void ez_template_extras() {
  // Only run this when not connected to a competition switch
  if (!pros::competition::is_connected()) {
    // PID Tuner
    // - after you find values that you're happy with, you'll have to set them in auton.cpp

    // Enable / Disable PID Tuner
    //  When enabled:
    //  * use A and Y to increment / decrement the constants
    //  * use the arrow keys to navigate the constants
    if (master.get_digital_new_press(DIGITAL_X))
      chassis.pid_tuner_toggle();

    // Trigger the selected autonomous routine
    if (master.get_digital(DIGITAL_B) && master.get_digital(DIGITAL_X)) {
      pros::motor_brake_mode_e_t preference = chassis.drive_brake_get();
      autonomous();
      chassis.drive_brake_set(preference);
    }

    // Allow PID Tuner to iterate
    chassis.pid_tuner_iterate();
  }

  // Disable PID Tuner when connected to a comp switch
  else {
    if (chassis.pid_tuner_enabled())
      chassis.pid_tuner_disable();
  }
}

/**
 * Runs the operator control code. This function will be started in its own task
 * with the default priority and stack size whenever the robot is enabled via
 * the Field Management System or the VEX Competition Switch in the operator
 * control mode.
 *
 * If no competition control is connected, this function will run immediately
 * following initialize().
 *
 * If the robot is disabled or communications is lost, the
 * operator control task will be stopped. Re-enabling the robot will restart the
 * task, not resume it from where it left off.
 */
void opcontrol() {
  // This is preference to what you like to drive on
  chassis.drive_brake_set(MOTOR_BRAKE_HOLD);

  while (true) {
    // Gives you some extras to make EZ-Template ezier
    ez_template_extras();

    chassis.opcontrol_arcade_standard(ez::SPLIT);

    // Set up OP controls for INTAKE                                                         
    if (master.get_digital(DIGITAL_R1)){
        setIntake(127);
    } else if (master.get_digital(DIGITAL_R2)){
        setIntake(-100);
    } else {
        setIntake(0);
    }

    // Set up OP controls for MOGO MECH
    if (master.get_digital(DIGITAL_L1)){
      extendMogo();
    } else if (master.get_digital(DIGITAL_L2)){
      retractMogo();
    }

    // Set up OP controls for LADYBROWN
    if (master.get_digital(DIGITAL_DOWN)){
      setLB(-80);
    } else if (master.get_digital(DIGITAL_UP)){
        if(ladybrown_Right.get_position() >= 1980){
          setLB(0);
          setLBbrake();
        } else {
          setLB(80);
        }
    } else if (master.get_digital(DIGITAL_RIGHT)){
      ladybrown_Right.move_absolute(425, 127);
      ladybrown_Left.move_absolute(-425, 127);
    } else {
      setLB(0);
      setLBbrake();
    }

    // Set up OP controls to prepare ring for LB
    if (master.get_digital(DIGITAL_Y)){
      
      for(int i = 0; i < 4; i++){ 
        intake.move(127);
        pros::delay(500);
        intake.move(0);
      }
      
      intake.move(-127);
      pros::delay(50);
      intake.move(0);
      master.rumble("-");
    }

    // Set up OP controls for DOINKER
    doinker.button_toggle(master.get_digital(DIGITAL_A));
    pros::delay(10);

    // Set up OP controls for INTAKE LIFTER
    intakeLifter.button_toggle(master.get_digital(DIGITAL_B));
    pros::delay(10);

    pros::delay(ez::util::DELAY_TIME);  // This is used for timer calculations!  Keep this ez::util::DELAY_TIME
  }
}
