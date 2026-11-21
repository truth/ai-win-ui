import { createElement } from "../../../runtime/index.ts";
import { Panel, Label, Icon, Divider } from "../../../runtime/index.ts";
import type { CalendarEvent } from "../data/mockData.ts";

export interface CalendarCardProps {
  events: CalendarEvent[];
  activeEventId: string;
  onSelectEvent: (eventId: string) => void;
  selectedFilter?: string;
  onToggleFilter?: () => void;
}

export function CalendarCard({
  events,
  activeEventId,
  onSelectEvent,
  selectedFilter = "Today",
  onToggleFilter,
}: CalendarCardProps) {
  return (
    <Panel
      direction="column"
      background="#FFFFFF"
      cornerRadius={18}
      padding={[18, 14, 18, 14]}
      borderColor="#E2E8F0"
      border={[1, 1, 1, 1]}
      width={260}
      minWidth={260}
      maxWidth={260}
    >
      {/* Header: Title & Today filter */}
      <Panel
        direction="row"
        alignItems="center"
        justifyContent="space-between"
        padding={0}
      >
        <Panel direction="column" padding={0}>
          <Label text="Calendar" fontSize={16} bold={true} color="#0F172A" />
          <Label text="6 events today" fontSize={10} color="#94A3B8" />
        </Panel>

        <Panel
          direction="row"
          alignItems="center"
          padding={[10, 4, 10, 4]}
          cornerRadius={12}
          background="#F8FAFC"
          borderColor="#E2E8F0"
          border={[1, 1, 1, 1]}
          spacing={4}
          onClick={onToggleFilter}
        >
          <Label text={selectedFilter} fontSize={11} color="#64748B" />
          <Icon name="chevron-down" size={12} color="#64748B" />
        </Panel>
      </Panel>

      <Panel height={14} padding={0} />

      {/* Timeline slots & lesson cards */}
      <Panel direction="column" spacing={8} padding={0}>
        {events.map((ev) => {
          const isActive = ev.id === activeEventId;
          return (
            <Panel
              key={ev.id}
              direction="row"
              alignItems="center"
              padding={0}
              spacing={8}
            >
              {/* Time column */}
              <Panel width={36} minWidth={36} padding={0}>
                <Label
                  text={ev.timeSlot}
                  fontSize={10}
                  bold={isActive}
                  color={isActive ? "#3751FE" : "#94A3B8"}
                />
              </Panel>

              {/* Event card */}
              <Panel
                direction="row"
                alignItems="center"
                flexGrow={1}
                padding={[10, 8, 10, 8]}
                cornerRadius={12}
                background={isActive ? "#4361EE" : "#F8FAFC"}
                spacing={8}
                onClick={() => onSelectEvent(ev.id)}
              >
                {/* Icon badge */}
                <Panel
                  direction="row"
                  alignItems="center"
                  justifyContent="center"
                  width={24}
                  height={24}
                  cornerRadius={12}
                  background={isActive ? "#6366F1" : "#EEF2F6"}
                >
                  <Icon
                    name="book-open"
                    size={12}
                    color={isActive ? "#FFFFFF" : "#64748B"}
                  />
                </Panel>

                {/* Event info */}
                <Panel direction="column" padding={0} flexGrow={1}>
                  <Label
                    text={ev.title}
                    fontSize={11}
                    bold={true}
                    color={isActive ? "#FFFFFF" : "#0F172A"}
                  />
                  <Label
                    text={`${ev.timeRange}, ${ev.roomInfo}`}
                    fontSize={9}
                    color={isActive ? "#E0E7FF" : "#94A3B8"}
                  />
                </Panel>
              </Panel>
            </Panel>
          );
        })}

        {/* Dashed timeline divider */}
        <Panel direction="row" alignItems="center" padding={[4, 0, 4, 0]} spacing={8}>
          <Panel width={36} minWidth={36} padding={0}>
            <Label text="11:00" fontSize={10} color="#94A3B8" />
          </Panel>
          <Panel flexGrow={1} height={1} background="#CBD5E1" padding={0} />
        </Panel>
      </Panel>
    </Panel>
  );
}

