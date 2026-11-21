import { createElement } from "../../../runtime/index.ts";
import { Panel, Label, Image } from "../../../runtime/index.ts";

export interface HeroBannerProps {
  studentName?: string;
  taskCount?: number;
  onReview?: () => void;
}

export function HeroBanner({
  studentName = "Grace",
  taskCount = 3,
  onReview,
}: HeroBannerProps) {
  return (
    <Panel
      direction="row"
      alignItems="center"
      justifyContent="space-between"
      background="#F8FAFC"
      cornerRadius={18}
      padding={[24, 18, 16, 20]}
      height={145}
    >
      {/* Left text column */}
      <Panel direction="column" justifyContent="center" padding={0} flexGrow={1}>
        <Label
          text={`Hello ${studentName}!`}
          fontSize={22}
          bold={true}
          color="#0F172A"
        />
        <Panel height={6} padding={0} />
        <Label
          text={`You have ${taskCount} new tasks. It is a lot of work for today! So let's start!`}
          fontSize={12}
          color="#64748B"
        />
        <Panel height={10} padding={0} />
        <Panel
          direction="row"
          alignItems="center"
          padding={0}
          onClick={onReview}
        >
          <Label
            text="review it"
            fontSize={12}
            bold={true}
            color="#3751FE"
          />
        </Panel>
      </Panel>

      {/* Right 3D student illustration */}
      <Panel
        direction="column"
        alignItems="center"
        justifyContent="center"
        width={180}
        height={130}
        padding={0}
      >
        <Image
          src="images/student_hero.png"
          width={170}
          height={125}
          stretch="uniform"
        />
      </Panel>
    </Panel>
  );
}

