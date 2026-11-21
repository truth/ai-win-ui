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
  timeLabel: string;
  timeRangeLabel: string;
  title: string;
  timeRange: string;
  roomInfo: string;
  icon: string;
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
  { id: "lessons", label: "Lessons", icon: "file-text" },
  { id: "schedule", label: "Schedule", icon: "calendar" },
  { id: "materials", label: "Materials", icon: "book-open" },
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
    { subject: "Algorithms\nstructures", percent: 92 },
    { subject: "Object\nprogram.", percent: 83 },
    { subject: "Database\nprogram.", percent: 78 },
    { subject: "Web\ndevelop.", percent: 97 },
    { subject: "Mobile\napplication", percent: 96 },
    { subject: "Machine\nlearning", percent: 89 },
  ],
  November: [
    { subject: "Algorithms\nstructures", percent: 95 },
    { subject: "Object\nprogram.", percent: 88 },
    { subject: "Database\nprogram.", percent: 82 },
    { subject: "Web\ndevelop.", percent: 91 },
    { subject: "Mobile\napplication", percent: 90 },
    { subject: "Machine\nlearning", percent: 85 },
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
    timeLabel: "10:00",
    timeRangeLabel: "10:00",
    title: "Electronics lesson",
    timeRange: "9.45-10.30",
    roomInfo: "21 lesson",
    icon: "graduation-cap",
  },
  {
    id: "c2",
    timeLabel: "11:00\n11:30",
    timeRangeLabel: "11:00",
    title: "Electronics lesson",
    timeRange: "11.00-11.40",
    roomInfo: "23 lesson",
    icon: "book-open",
  },
  {
    id: "c3",
    timeLabel: "12:00\n12:30",
    timeRangeLabel: "12:00",
    title: "Robotics lesson",
    timeRange: "12.00-12.45",
    roomInfo: "23 lesson",
    icon: "sliders",
  },
  {
    id: "c4",
    timeLabel: "13:30\n14:00",
    timeRangeLabel: "13:30",
    title: "C++ lesson",
    timeRange: "13.45-14.30",
    roomInfo: "21 lesson",
    icon: "code",
  },
];

export const UPCOMING_EVENTS: UpcomingEvent[] = [
  {
    id: "u1",
    title: 'The main event in your life "Robot Fest" will coming soon in...',
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
