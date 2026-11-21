import { createElement } from "../../../runtime/index.ts";
import { Panel, Label, Icon, Image } from "../../../runtime/index.ts";
import { UPCOMING_EVENTS, type UpcomingEvent } from "../data/mockData.ts";

export interface UpcomingEventsCardProps {
  onSeeAll?: () => void;
  onEventClick?: (eventId: string) => void;
}

export function UpcomingEventsCard({
  onSeeAll,
  onEventClick,
}: UpcomingEventsCardProps) {
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
      {/* Title & See All */}
      <Panel
        direction="row"
        alignItems="center"
        justifyContent="space-between"
        padding={0}
      >
        <Label text="Upcoming events" fontSize={15} bold={true} color="#0F172A" />
        <Panel padding={0} onClick={onSeeAll}>
          <Label text="See all" fontSize={11} bold={true} color="#3751FE" />
        </Panel>
      </Panel>

      <Panel height={12} padding={0} />

      {/* Events List */}
      <Panel direction="column" spacing={12} padding={0}>
        {UPCOMING_EVENTS.map((ev) => (
          <Panel
            key={ev.id}
            direction="row"
            alignItems="center"
            spacing={10}
            padding={[6, 4, 6, 4]}
            cornerRadius={12}
            background="#F8FAFC"
            onClick={() => onEventClick?.(ev.id)}
          >
            {/* Thumbnail */}
            <Image
              src={ev.thumbnail}
              width={34}
              height={34}
              cornerRadius={17}
              stretch="uniform"
            />

            {/* Info */}
            <Panel direction="column" flexGrow={1} padding={0}>
              <Label
                text={ev.title}
                fontSize={11}
                bold={true}
                color="#0F172A"
                ellipsis={true}
              />
              <Panel height={4} padding={0} />
              <Panel direction="row" alignItems="center" spacing={4} padding={0}>
                <Icon name="clock" size={10} color="#94A3B8" />
                <Label
                  text={`${ev.date}   ${ev.time}`}
                  fontSize={9}
                  color="#94A3B8"
                />
              </Panel>
            </Panel>

            {/* More Options */}
            <Panel
              direction="row"
              alignItems="center"
              justifyContent="center"
              width={22}
              height={22}
              padding={0}
            >
              <Icon name="more-vertical" size={13} color="#94A3B8" />
            </Panel>
          </Panel>
        ))}
      </Panel>
    </Panel>
  );
}

