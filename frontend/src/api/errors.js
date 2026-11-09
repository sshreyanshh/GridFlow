export class ApiError extends Error {
  constructor(code, message, details = null, status = 0) { super(message); this.name = 'ApiError'; this.code = code; this.details = details; this.status = status; }
}
