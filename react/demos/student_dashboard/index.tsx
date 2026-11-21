import {
  createElement,
  useState,
  useMemo,
  render,
  setWindowTitle,
} from "../../runtime/index.ts";
import { Panel, ScrollViewer } from "../../runtime/index.ts";

import { Sidebar } from "./components/Sidebar.tsx";
import { TopBar } from "./components/TopBar.tsx";
import { HeroBanner } from "./components/HeroBanner.tsx";
import { PerformanceCard } from "./components/PerformanceCard.tsx";
import { MyVisitCard } from "./components/MyVisitCard.tsx";
import { LinkedTeachersCard } from "./components/LinkedTeachersCard.tsx";
import { CalendarCard } from "./components/CalendarCard.tsx";
import { UpcomingEventsCard } from "./components/UpcomingEventsCard.tsx";

import {
  PERFORMANCE_DATA,
  VISIT_DATA,
  CALENDAR_EVENTS,
} from "./data/mockData.ts";

export function StudentDashboardApp() {
  const [activeTab, setActiveTab] = useState("dashboard");
  const [searchText, setSearchText] = useState("");
  const [language, setLanguage] = useState("ENG");
  const [month, setMonth] = useState("December");
  const [showAllLessons, setShowAllLessons] = useState(false);
  const [activeEventId, setActiveEventId] = useState("c1");
  const [statusMessage, setStatusMessage] = useState("");

  const toggleMonth = () => {
    setMonth((m) => (m === "December" ? "November" : "December"));
  };

  const toggleLanguage = () => {
    setLanguage((l) => (l === "ENG" ? "CHN" : "ENG"));
  };

  // Filter performance bars by search query if any
  const performanceData = useMemo(() => {
    const raw = PERFORMANCE_DATA[month] || PERFORMANCE_DATA.December;
    if (!searchText.trim()) return raw;
    const q = searchText.toLowerCase();
    return raw.filter((p) => p.subject.toLowerCase().includes(q));
  }, [month, searchText]);

  const visitData = useMemo(() => {
    const raw = VISIT_DATA[month] || VISIT_DATA.December;
    if (!searchText.trim()) return raw;
    const q = searchText.toLowerCase();
    return raw.filter((v) => v.subject.toLowerCase().includes(q));
  }, [month, searchText]);

  return (
    <Panel
      direction="row"
      width={1240}
      height={780}
      minWidth={1080}
      minHeight={680}
      background="#3751FE"
      padding={0}
      alignItems="stretch"
    >
      {/* 1. Left Blue Navigation Sidebar */}
      <Sidebar
        activeTab={activeTab}
        onSelectTab={(tabId) => {
          setActiveTab(tabId);
          setWindowTitle(`Dashboard for student - ${tabId}`);
        }}
        onLogOut={() => {
          setStatusMessage("Logged out successfully");
        }}
      />

      {/* 2. Main Canvas (White rounded background) */}
      <Panel
        direction="column"
        flexGrow={1}
        background="#F8FAFC"
        cornerRadius={28}
        margin={[8, 8, 8, 8]}
        padding={[20, 20, 24, 20]}
        alignItems="stretch"
      >
        {/* Top Header Bar */}
        <TopBar
          searchText={searchText}
          onSearchChange={setSearchText}
          language={language}
          onToggleLanguage={toggleLanguage}
        />

        <Panel height={14} padding={0} />

        {/* Content Body: 3-column responsive layout */}
        <ScrollViewer scrollY={true} scrollX={false} flexGrow={1} padding={0}>
          <Panel direction="row" alignItems="stretch" spacing={16} padding={0}>
            {/* Column 1: Main Overview (Hero + Performance + Teachers) */}
            <Panel
              direction="column"
              flexGrow={1.4}
              spacing={14}
              padding={0}
              alignItems="stretch"
            >
              <HeroBanner
                studentName="Grace"
                taskCount={3}
                onReview={() => {
                  setActiveTab("lessons");
                }}
              />

              <PerformanceCard
                data={performanceData}
                selectedMonth={month}
                onToggleMonth={toggleMonth}
                showAllLessons={showAllLessons}
                onToggleAllLessons={() => setShowAllLessons((s) => !s)}
              />

              <LinkedTeachersCard
                onSeeAll={() => setActiveTab("lessons")}
                onContactTeacher={(id, action) => {
                  setStatusMessage(`Contacting teacher ${id} via ${action}`);
                }}
              />
            </Panel>

            {/* Column 2: My Visit (Circular Progress Donut Rings) */}
            <Panel direction="column" spacing={14} padding={0}>
              <MyVisitCard
                data={visitData}
                selectedMonth={month}
                onToggleMonth={toggleMonth}
              />
            </Panel>

            {/* Column 3: Calendar Schedule & Upcoming Events */}
            <Panel direction="column" spacing={14} padding={0}>
              <CalendarCard
                events={CALENDAR_EVENTS}
                activeEventId={activeEventId}
                onSelectEvent={setActiveEventId}
                onToggleFilter={() => {
                  setStatusMessage("Filtered calendar events");
                }}
              />

              <UpcomingEventsCard
                onSeeAll={() => setActiveTab("schedule")}
                onEventClick={(id) => {
                  setStatusMessage(`Viewing event details for ${id}`);
                }}
              />
            </Panel>
          </Panel>
        </ScrollViewer>
      </Panel>
    </Panel>
  );
}

setWindowTitle("Dashboard for student - Light mode");
render(<StudentDashboardApp />);

