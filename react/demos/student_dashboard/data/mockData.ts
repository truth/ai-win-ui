export interface PerformanceBar {
  subject: string;
  shortName: string;
  score: number;
}

export interface VisitStat {
  subject: string;
  percent: number;
}

export interface Teacher {
  id: string;
  name: string;
  role: string;
  subject: string;
  avatar: string;
}

export interface CalendarEvent {
  id: string;
  timeSlot: string;
  title: string;
  timeRange: string;
  roomInfo: string;
  active?: boolean;
}

export interface UpcomingEvent {
  id: string;
  title: string;
  date: string;
  time: string;
  thumbnail: string;
}

export interface NavTab {
  id: string;
  label: string;
  icon: string;
}

export const NAV_TABS: NavTab[] = [
  { id: "dashboard", label: "Dashboard", icon: "dashboard" },
  { id: "lessons", label: "Lessons", icon: "book" },
  { id: "schedule", label: "Schedule", icon: "calendar" },
  { id: "materials", label: "Materials", icon: "file-text" },
  { id: "forum", label: "Forum", icon: "message-square" },
  { id: "assessments", label: "Assessments", icon: "award" },
  { id: "settings", label: "Settings", icon: "settings" },
];

export const PERFORMANCE_DATA: Record<string, PerformanceBar[]> = {
  December: [
    { subject: "Algorithms structures", shortName: "Algorithms\nstructures", score: 85.3 },
    { subject: "Object program", shortName: "Object\nprogram.", score: 64.7 },
    { subject: "Database program", shortName: "Database\nprogram.", score: 84.2 },
    { subject: "Web develop", shortName: "Web\ndevelop.", score: 45.6 },
    { subject: "Mobile application", shortName: "Mobile\napplicat.", score: 43.5 },
    { subject: "Machine learning", shortName: "Machine\nlearning", score: 74.4 },
  ],
  November: [
    { subject: "Algorithms structures", shortName: "Algorithms\nstructures", score: 78.0 },
    { subject: "Object program", shortName: "Object\nprogram.", score: 82.5 },
    { subject: "Database program", shortName: "Database\nprogram.", score: 71.0 },
    { subject: "Web develop", shortName: "Web\ndevelop.", score: 68.4 },
    { subject: "Mobile application", shortName: "Mobile\napplicat.", score: 55.0 },
    { subject: "Machine learning", shortName: "Machine\nlearning", score: 80.2 },
  ],
};

export const VISIT_DATA: Record<string, VisitStat[]> = {
  December: [
    { subject: "Algorithms structures", percent: 92 },
    { subject: "Object program.", percent: 83 },
    { subject: "Database program.", percent: 78 },
    { subject: "Web develop.", percent: 97 },
    { subject: "Mobile application", percent: 96 },
    { subject: "Machine learning", percent: 89 },
  ],
  November: [
    { subject: "Algorithms structures", percent: 95 },
    { subject: "Object program.", percent: 88 },
    { subject: "Database program.", percent: 82 },
    { subject: "Web develop.", percent: 91 },
    { subject: "Mobile application", percent: 90 },
    { subject: "Machine learning", percent: 85 },
  ],
};

export const TEACHERS: Teacher[] = [
  {
    id: "t1",
    name: "Mary Johnson",
    role: "(mentor)",
    subject: "Science",
    avatar: "images/teacher_mary.png",
  },
  {
    id: "t2",
    name: "James Brown",
    role: "",
    subject: "Foreign language (Chinese)",
    avatar: "images/teacher_james.png",
  },
];

export const CALENDAR_EVENTS: CalendarEvent[] = [
  {
    id: "c1",
    timeSlot: "10:00",
    title: "Electronics lesson",
    timeRange: "9.45-10.30",
    roomInfo: "21 lesson",
    active: true,
  },
  {
    id: "c2",
    timeSlot: "11:30",
    title: "Electronics lesson",
    timeRange: "11.00-11.40",
    roomInfo: "23 lesson",
    active: false,
  },
  {
    id: "c3",
    timeSlot: "12:30",
    title: "Robotics lesson",
    timeRange: "12.00-12.45",
    roomInfo: "23 lesson",
    active: false,
  },
  {
    id: "c4",
    timeSlot: "13:30",
    title: "C++ lesson",
    timeRange: "13.45-14.30",
    roomInfo: "21 lesson",
    active: false,
  },
];

export const UPCOMING_EVENTS: UpcomingEvent[] = [
  {
    id: "u1",
    title: "The main event in your life \"Robot Fest\" will coming soon in...",
    date: "14 December 2023",
    time: "12.00 pm",
    thumbnail: "images/event_robot.png",
  },
  {
    id: "u2",
    title: "Webinar of new tools in Minecraft",
    date: "21 December 2023",
    time: "11.00 pm",
    thumbnail: "images/event_minecraft.png",
  },
];

