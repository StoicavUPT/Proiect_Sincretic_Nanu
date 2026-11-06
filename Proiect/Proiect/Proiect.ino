
#include <LiquidCrystal.h>

LiquidCrystal lcd(16, 17, 18, 19, 21, 22);

#define BTN_OK      25
#define BTN_CANCEL  26
#define BTN_PLUS    27
#define BTN_MINUS   32

const unsigned long debounceDelay = 40;

struct Button {
  int pin;
  bool lastReading;
  bool stableState;
  unsigned long lastChange;
};

Button buttons[] = {
  {BTN_OK, HIGH, HIGH, 0},
  {BTN_CANCEL, HIGH, HIGH, 0},
  {BTN_PLUS, HIGH, HIGH, 0},
  {BTN_MINUS, HIGH, HIGH, 0}
};

float T_SET = 50.0;

int heatingTime = 60;
int holdingTime = 120;
int coolingTime = 60;

float Kp = 2.0;
float Ki = 0.5;
float Kd = 1.0;

const char* menuItems[] = {
  "Start program",
  "Set temperature",
  "Heating time",
  "Holding time",
  "Cooling time",
  "PID settings",
  "Save settings",
  "Exit"
};

const int MENU_SIZE = 8;

const char* pidNames[] = {
  "Kp",
  "Ki",
  "Kd"
};

enum SystemState {
  MAIN_MENU,
  EDIT_PARAMETER,
  PID_MENU,
  PID_EDIT,
  RUNNING
};

SystemState currentState = MAIN_MENU;

int menuIndex = 0;
int selectedPID = 0;

float oldPIDValue = 0.0;
float oldParameterValue = 0.0;

bool disturbance = false;
bool programStarted = false;

bool pressed(int index) {
  Button &b = buttons[index];

  bool reading = digitalRead(b.pin);

  if (reading != b.lastReading) {
    b.lastChange = millis();
    b.lastReading = reading;
  }

  if (millis() - b.lastChange > debounceDelay) {
    if (reading != b.stableState) {
      b.stableState = reading;

      if (b.stableState == LOW) {
        return true;
      }
    }
  }

  return false;
}

void displayMenu() {
  lcd.clear();

  int firstItem = menuIndex;

  if (firstItem >= MENU_SIZE - 1) {
    firstItem = MENU_SIZE - 2;
  }

  lcd.setCursor(0, 0);
  lcd.print(menuIndex == firstItem ? "> " : "  ");
  lcd.print(menuItems[firstItem]);

  lcd.setCursor(0, 1);
  lcd.print(menuIndex == firstItem + 1 ? "> " : "  ");
  lcd.print(menuItems[firstItem + 1]);
}

void handleMainMenu(bool ok, bool cancel, bool plus, bool minus) {
  if (plus) {
    menuIndex++;

    if (menuIndex >= MENU_SIZE) {
      menuIndex = 0;
    }

    displayMenu();
  }

  if (minus) {
    menuIndex--;

    if (menuIndex < 0) {
      menuIndex = MENU_SIZE - 1;
    }

    displayMenu();
  }

  if (ok) {
    switch (menuIndex) {
      case 0:
        startProgram();
        break;

      case 1:
      case 2:
      case 3:
      case 4:
        oldParameterValue = getParameterValue();
        currentState = EDIT_PARAMETER;
        displayParameter();
        break;

      case 5:
        selectedPID = 0;
        currentState = PID_MENU;
        displayPIDMenu();
        break;

      case 6:
        saveSettings();
        break;

      case 7:
        menuIndex = 0;
        displayMenu();
        break;
    }
  }

  if (cancel) {
    menuIndex = 0;
    displayMenu();
  }
}

float getParameterValue() {
  switch (menuIndex) {
    case 1: return T_SET;
    case 2: return heatingTime;
    case 3: return holdingTime;
    case 4: return coolingTime;
  }

  return 0;
}

void setParameterValue(float value) {
  switch (menuIndex) {
    case 1:
      T_SET = value;
      break;

    case 2:
      heatingTime = (int)value;
      break;

    case 3:
      holdingTime = (int)value;
      break;

    case 4:
      coolingTime = (int)value;
      break;
  }
}

void displayParameter() {
  lcd.clear();

  switch (menuIndex) {
    case 1:
      lcd.print("Temperature:");
      lcd.setCursor(0, 1);
      lcd.print(T_SET, 1);
      lcd.print(" C");
      break;

    case 2:
      lcd.print("Heating time:");
      lcd.setCursor(0, 1);
      lcd.print(heatingTime);
      lcd.print(" sec");
      break;

    case 3:
      lcd.print("Holding time:");
      lcd.setCursor(0, 1);
      lcd.print(holdingTime);
      lcd.print(" sec");
      break;

    case 4:
      lcd.print("Cooling time:");
      lcd.setCursor(0, 1);
      lcd.print(coolingTime);
      lcd.print(" sec");
      break;
  }
}

void changeParameter(int direction) {
  float value = getParameterValue();

  if (menuIndex == 1) {
    value += direction;
    value = constrain(value, 20.0f, 100.0f);
  } else {
    value += direction * 10;
    value = constrain(value, 10.0f, 3600.0f);
  }

  setParameterValue(value);
  displayParameter();
}

void handleParameterEdit(bool ok, bool cancel, bool plus, bool minus) {
  if (plus) changeParameter(1);
  if (minus) changeParameter(-1);

  if (ok) {
    currentState = MAIN_MENU;
    displayMenu();
  }

  if (cancel) {
    setParameterValue(oldParameterValue);
    currentState = MAIN_MENU;
    displayMenu();
  }
}

float getPIDValue() {
  if (selectedPID == 0) return Kp;
  if (selectedPID == 1) return Ki;
  return Kd;
}

void setPIDValue(float value) {
  if (selectedPID == 0) {
    Kp = value;
  } else if (selectedPID == 1) {
    Ki = value;
  } else {
    Kd = value;
  }
}

void displayPIDMenu() {
  lcd.clear();

  int firstItem = selectedPID;

  if (firstItem == 2) {
    firstItem = 1;
  }

  lcd.setCursor(0, 0);
  lcd.print(selectedPID == firstItem ? "> " : "  ");
  lcd.print(pidNames[firstItem]);
  lcd.print(": ");
  lcd.print(firstItem == 0 ? Kp : firstItem == 1 ? Ki : Kd, 2);

  lcd.setCursor(0, 1);
  lcd.print(selectedPID == firstItem + 1 ? "> " : "  ");
  lcd.print(pidNames[firstItem + 1]);
  lcd.print(": ");
  lcd.print(firstItem + 1 == 0 ? Kp : firstItem + 1 == 1 ? Ki : Kd, 2);
}

void displayPIDValue() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Edit ");
  lcd.print(pidNames[selectedPID]);
  lcd.setCursor(0, 1);
  lcd.print(getPIDValue(), 2);
}

void changePIDValue(int direction) {
  float value = getPIDValue();

  value += direction * 0.1f;
  value = max(0.0f, value);

  setPIDValue(value);
  displayPIDValue();
}

void handlePIDMenu(bool ok, bool cancel, bool plus, bool minus) {
  if (plus) {
    selectedPID++;

    if (selectedPID > 2) selectedPID = 0;

    displayPIDMenu();
  }

  if (minus) {
    selectedPID--;

    if (selectedPID < 0) selectedPID = 2;

    displayPIDMenu();
  }

  if (ok) {
    oldPIDValue = getPIDValue();
    currentState = PID_EDIT;
    displayPIDValue();
  }

  if (cancel) {
    currentState = MAIN_MENU;
    displayMenu();
  }
}

void handlePIDEdit(bool ok, bool cancel, bool plus, bool minus) {
  if (plus) changePIDValue(1);
  if (minus) changePIDValue(-1);

  if (ok) {
    currentState = PID_MENU;
    displayPIDMenu();
  }

  if (cancel) {
    setPIDValue(oldPIDValue);
    currentState = PID_MENU;
    displayPIDMenu();
  }
}

void startProgram() {
  programStarted = true;
  disturbance = false;
  currentState = RUNNING;
  displayRunningScreen();
}

void handleRunningState(bool ok, bool cancel) {
  if (ok) {
    disturbance = !disturbance;
    displayRunningScreen();
  }

  if (cancel) {
    stopProgram();
  }
}

void stopProgram() {
  programStarted = false;
  disturbance = false;
  currentState = MAIN_MENU;
  menuIndex = 0;
  displayMenu();
}

void displayRunningScreen() {
}

void updateProcess() {
}

void heatingStage() {
}

void holdingStage() {
}

void coolingStage() {
}

void calculatePID() {
}

void updatePIDOutput() {
}

float readTemperature() {
  return 0.0;
}

float filterTemperature(float temperature) {
  return temperature;
}

int generateRandom() {
  return 0;
}

float applyDisturbance(float temperature) {
  return temperature;
}

void saveSettings() {
}

void loadSettings() {
}

void setup() {
  Serial.begin(115200);

  lcd.begin(16, 2);

  for (int i = 0; i < 4; i++) {
    pinMode(buttons[i].pin, INPUT_PULLUP);
  }

  loadSettings();

  lcd.setCursor(0, 0);
  lcd.print("Temperature");

  lcd.setCursor(0, 1);
  lcd.print("Control System");

  delay(1500);

  displayMenu();
}

void loop() {
  bool ok = pressed(0);
  bool cancel = pressed(1);
  bool plus = pressed(2);
  bool minus = pressed(3);

  switch (currentState) {
    case MAIN_MENU:
      handleMainMenu(ok, cancel, plus, minus);
      break;

    case EDIT_PARAMETER:
      handleParameterEdit(ok, cancel, plus, minus);
      break;

    case PID_MENU:
      handlePIDMenu(ok, cancel, plus, minus);
      break;

    case PID_EDIT:
      handlePIDEdit(ok, cancel, plus, minus);
      break;

    case RUNNING:
      handleRunningState(ok, cancel);
      updateProcess();
      break;
  }
}
