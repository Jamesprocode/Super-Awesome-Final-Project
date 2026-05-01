/** IEC standby-style power symbol (shared by mapping + detailed bypass). */
export function PowerIcon() {
  return (
    <svg
      xmlns="http://www.w3.org/2000/svg"
      width="22"
      height="22"
      viewBox="0 0 24 24"
      aria-hidden
    >
      <g transform="translate(0 -2)">
        <g transform="rotate(180 12 12)">
          <path
            d="M12 7v10"
            fill="none"
            stroke="currentColor"
            strokeWidth="2.2"
            strokeLinecap="round"
          />
          <path
            d="M7.94 13.93a6 6 0 1 1 8.12 0"
            fill="none"
            stroke="currentColor"
            strokeWidth="2.2"
            strokeLinecap="round"
          />
        </g>
      </g>
    </svg>
  )
}
