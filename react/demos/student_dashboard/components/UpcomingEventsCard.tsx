import { createElement } from "../../../runtime/index.ts";
import { Panel, Label, Icon, Image } from "../../../runtime/index.ts";
import { UPCOMING_EVENTS } from "../data/mockData.ts";

export interface UpcomingEventsCardProps {
  onSeeAll?: () => void;
  onEventClick?: (eventId: string) => void;
}

export function UpcomingEventsCard({
  onSeeAll,
  onEventClick,
}: UpcomingEventsCardProps) {
  return (
    <Panel direction="column" padding={0} spacing={8} alignItems="stretch">
      {/* External Title Row */}
      <Panel
        direction="row"
        alignItems="center"
        justifyContent="space-between"
        height={22}
        padding={0}
      >
        <Label text="Upcoming events" fontSize={13} bold={true} color="#1E2538" flexGrow={1} />
        <Panel direction="row" alignItems="center" justifyContent="end" width={46} minWidth={46} height={22} padding={0} onClick={onSeeAll}>
          <Label text="See all" fontSize={11} fontWeight={500} color="#3D509B" />
        </Panel>
      </Panel>

      {/* Separate White Card per Event */}
      <Panel direction="column" spacing={8} padding={0} alignItems="stretch">
        {UPCOMING_EVENTS.map((ev) => (
          <Panel
            key={ev.id}
            direction="row"
            alignItems="center"
            height={58}
            spacing={9}
            padding={{ left: 10, top: 8, right: 6, bottom: 8 }}
            cornerRadius={14}
            background="#FFFFFF"
            onClick={() => onEventClick?.(ev.id)}
          >
            {/* Thumbnail */}
            <Image
              src={ev.thumbnail}
              width={32}
              height={32}
              cornerRadius={16}
              stretch="uniformToFill"
            />

            {/* Event Title + Date/Time */}
            <Panel direction="column" flexGrow={1} flexBasis={0} padding={0} spacing={3}>
              <Label
                text={ev.title}
                fontSize={10}
                bold={true}
                color="#1E2538"
                ellipsis={true}
              />
              <Panel direction="row" alignItems="center" spacing={4} padding={0}>
                <Icon name="clock" size={9} color="#8E9BAE" />
                <Label
                  text={`${ev.date}   ${ev.time}`}
                  fontSize={8.5}
                  color="#64748B"
                  ellipsis={true}
                />
              </Panel>
            </Panel>

            {/* 3-Dot More Icon */}
            <Panel
              direction="row"
              alignItems="center"
              justifyContent="center"
              width={18}
              height={24}
              padding={0}
            >
              <Icon name="more-vertical" size={12} color="#3D509B" />
            </Panel>
          </Panel>
        ))}
      </Panel>
    </Panel>
  );
}
